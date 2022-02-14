#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "arrays/taskSet.h"
#include "arrays/int-array.h"
#include "tools/receiver.h"
#include "tools/printer.h"


// ---------- GLOBAL VARIABLES ---------- //



// MAX AMOUNTS!
#define MAX_LAMPORT_QUEGE_SIZE 100
#define MAX_TASK_QUEGE_SIZE 100

// --- ROLE ENUM
int ROLE;
const int LIB = 1, CONAN = 2;

// --- MUTEX
pthread_mutex_t slipLock, washLock;

// --- PROCESS GLOBAL VARIABLES
int pid, pcount;
int libCount, conanCount;

// --- INTERVALS
int LIB_TIME, TASK_TIME, WASH_TIME;

// --- LAMPORT
int LAMPORT_CLOCK;

// --- MESSAGE TAGS
#define TASK_REQ 1
#define TASK_ACK 2
#define TASK_REL 3
#define TASK_NEW 4
#define SLIP_REQ 5
#define SLIP_ACK 6
#define SLIP_REL 7
#define WASH_REQ 8
#define WASH_ACK 9
#define WASH_REL 10

// --- TASK LIST
int_array *libMaxTaskID;
TaskSet *taskSet;
Task ongoingTask;
TaskSet *onlyMyTasks;

// --- CONAN STATE FLAGS AND COUNTER
#define FREE 0 // so this evaluates to false
#define NEED_SLIPS 1
#define HAVE_SLIPS 2

// --- SLIPS ZONE
int_array *slipQuege, *washQuege;
int slipQuegeCapacity, washQuegeCapacity;
volatile int BUSY; // to use with above
volatile int washToRequest = 0; // to use as BUSY between threads - mutex this
volatile int myDirtySlipCount = 0, myCleanSlips = 0;
int myLastSlipInLamportValue = -1, myLastWashInLamportValue = -1;

// --- MPI TYPE
MPI_Datatype MPI_TASK_STRUCT;
ReceiverData mainReceiver;

// VARIABLES BOUND TO FUNCTIONS / BUFFERS / OTHERS
MPI_Status status;
MPI_Request bcastReq; // init in main
int taskData[4];



// ---------- TOOLS ---------- //



/*
 * update lamport clock after receiving a message
 */
int updateLamportClock(int otherClock){
    if(otherClock > LAMPORT_CLOCK) {
        LAMPORT_CLOCK = otherClock;
    }
    LAMPORT_CLOCK++;
}

/*
 * compare process priorities; 1 means first passed is more important, 0 means otherwise
 */
int comparePriority(int pid1, int lamport1, int pid2, int lamport2) {

    if(lamport1 < lamport2){
    return 1;

    } else if (lamport1 == lamport2){

        if(pid1 < pid2){
            return 1;
        } else {
            return 0;
        }

    } else {

        return 0;
    }
}
/*
 * Chack if tak with given libID and taskID is pending; return pointer and info if it was found. 
 * outIndex = i if true, 
 * -1 if false, 
 * -2 if it is current task.
 * -3 if it is finished and pending flag has been raised
 * taskSet.max+index if it is on notYetReceivedTaskList
 * returnet pointer = 0 if false as well
 */
 Task *findTaskByID(int libID, int taskID, bool pendingOnly, int *outIndex){
    Task *task = findTask(libID, taskID, taskSet, outIndex);
    if(*outIndex >= 0){
        return task;
    }
    if(BUSY && libID == ongoingTask.libID && taskID == ongoingTask.taskID){
        *outIndex = -2;
        return &ongoingTask;
    }
    *outIndex = -1;
    return 0;
}

int *get_librarian_max_taskID(int libID, int taskID){
    int c = libCount * 2;
    for(int i=0; i<c; i+=2){
        if(libMaxTaskID->array[i] == libID){
            return &libMaxTaskID->array[i+1];
        }
    }
}

void update_librarian_max_taskID(int libID, int taskID){
    int c = libCount * 2;
    for(int i=0; i<c; i+=2){
        if(libMaxTaskID->array[i] == libID && libMaxTaskID->array[i+1] < taskID){
            libMaxTaskID->array[i+1] = taskID;
        }
    }
}



// ---------- LIBRARIAN ---------- //



int getRandomArray(int *arr, int len, int minValue) {
    int arrLen = 0;
    for (int i=minValue; i<len+minValue; i++) {
        if( rand()%2 == 0 ){
            arr[arrLen++] = i;
        }
    }
    return arrLen;
}

/*
 * main loop for librarian
 * uses global MPI_Request bcastReq;
 */
void runLibrarian_Loop() {

    // buffer
    int messageMaxSize = 5 + conanCount;
    int messageMaxSizeByte = messageMaxSize * sizeof(int) + MPI_BSEND_OVERHEAD;
    int bufSizeByte = messageMaxSizeByte * conanCount;

    int *buf = malloc(bufSizeByte);
    MPI_Buffer_attach(buf, bufSizeByte);

    ROLE = LIB;

    taskData[0] = 1; //lamport
    taskData[1] = pid; //libID
    taskData[2] = 0; //taskID

    int pidArr[conanCount];

    time_t t;
    srand((unsigned) time(&t));

    while(true){
        taskData[0]++;
        taskData[2]++;
        // Broadcast lamport, libID, taskID

        // 'randomly' draw proecss to involve
        do {
            taskData[3] = getRandomArray(pidArr, conanCount, libCount);
        } while (taskData[3] <= 0);
        
        memcpy(&taskData[4], pidArr, taskData[3] * sizeof(int));
        int msgLen = 5 + taskData[3];


        printNewTask(STYLE_NEW, pid, LAMPORT_CLOCK, P_BROADCASTING, taskData[1], taskData[2], &taskData[4], taskData[3]);

        for (int i=0; i<taskData[3]; i++) {
            MPI_Bsend(taskData, msgLen, MPI_INT, pidArr[i], TASK_NEW, MPI_COMM_WORLD);
        }

        sleep(LIB_TIME);
    }

    MPI_Buffer_detach(buf, &bufSizeByte);
    free(buf);
}



// ---------- CONAN YET WITHOUT SLIPS WITH HIS SWORD HANGING OUT ---------- //




int find_any_lamport_by_value(int originLamport, int id, int_array *set){
    for(int i=0; i<set->top; i++){
        int index = i * set->elemSize;
        if(set->array[index] == originLamport &&
            set->array[index+1] == id){
            return i;
        }
    }
    return -1;
}

int find_my_lamport(int occurence, int_array *set, int *lamport_out) {
    int occ_couter= 0;
    for(int i=0; i<set->top; i++){
        int index = i * set->elemSize;
        if(set->array[index+1] == pid){
            if(occ_couter++ >= occurence) {
                *lamport_out = set->array[index];
                return i;
            }
        }
    }
    return -1;
}

int find_my_lamport_by_value(int searched_lamport, int_array *set, int *lamport_out) {
    for(int i=0; i<set->top; i++){
        int index = i * set->elemSize;
        if(set->array[index+1] == pid && set->array[index] == searched_lamport){
            *lamport_out = set->array[index];
            return i;
        }
    }
    return -1;
}

void broadcast(int *send_buffer, int libID, int buffer_size, int TAG) {
    char *sendTAG;
    char *sendOP;
    switch(TAG){
        case SLIP_REQ:
        case WASH_REQ:
            sendTAG = P_REQ;
            break;
        case SLIP_REL:
        case WASH_REL:
            sendTAG = P_REL;
            break;
    }
    switch(TAG){
        case SLIP_REQ:
        case SLIP_REL:
            sendOP = P_SLIP;
            break;
        case WASH_REQ:
        case WASH_REL:
            sendOP = P_WASH;
            break;
    }
    // TODO for each client in group
    for(int i=libCount; i<pcount; i++){
        if(i != pid){
            printCommSend(pid, LAMPORT_CLOCK, sendTAG, sendOP, i);
            MPI_Bsend(send_buffer, buffer_size, MPI_INT, i, TAG, MPI_COMM_WORLD);
        }
    }
}

void sendLamport_ACK(int target, int originalREQLamport, int TAG) {
    int send_buffer[2] = {++LAMPORT_CLOCK, originalREQLamport};
    printCommSend(pid, LAMPORT_CLOCK, P_ACK, P_SLIP, target);
    MPI_Bsend(send_buffer, 2, MPI_INT, target, TAG, MPI_COMM_WORLD);
}

int getMinimumACK(int_array *set, int setCapacity) {
    // mainly search for duplicates
    int duplicates = 0;
    for(int i=0; i<setCapacity; i++){ // i = comparedIndex
        for(int j=i+1; j<setCapacity; j++){
            
            int *elem1 = get_int(i, set);
            int *elem2 = get_int(j, set);
            if(elem1[1] == elem2[1]){
                duplicates++;
            }
            
        }
    }
   
    return conanCount-(setCapacity-1)+duplicates; // N-(K-1)+D
}

bool checkIfElementIsIn(int index, int_array *set, int setCapacity, int requiredACK) {

    if (index < setCapacity && get_int(index, set)[2] >= requiredACK){
        return true;
    }
    return false;

}

int myNextLamportIsIn(int startingIndex, int_array *set, int setCap, int requiredACK) {
    for(int i=startingIndex; i<setCap; i++){
        int *elem = get_int(i, set);
        if( elem[1] == pid &&
            checkIfElementIsIn(i, set, setCap, requiredACK) ) {
            if(elem[0] > myLastSlipInLamportValue){
                myLastSlipInLamportValue = elem[0];
            }
            return i;
        } 
    }
    return -1;
}

int myNextLamportWentIn(int startingIndex, int_array *set, int setCap, int requiredACK) {
    
    for(int i=startingIndex; i<setCap; i++){
        int *elem = get_int(i, set);
       
        if( elem[1] == pid &&
            checkIfElementIsIn(i, set, setCap, requiredACK) &&
            elem[0] > myLastSlipInLamportValue) {
                myLastSlipInLamportValue = elem[0];
                return i;
        } 
    }
    return -1;
}

void forEachElementEntered(int TAG, int_array *quege, int quegeCap);

void broadcastLamport_REQ(int_array *quege, int quegeCap, int TAG) {
    int send_buffer[1] = {++LAMPORT_CLOCK};
    switch(TAG) {
        case SLIP_REQ:
            printSlip(STYLE_SLIP, pid, LAMPORT_CLOCK, P_REQUESTING, LAMPORT_CLOCK, pid);
            break;
        case WASH_REQ:
            printWash(STYLE_WASH, pid, LAMPORT_CLOCK, P_REQUESTING, LAMPORT_CLOCK, pid);
            break;
    }
    broadcast(send_buffer, 0, 2, TAG);

    int toAdd[3] = {LAMPORT_CLOCK, pid, 0};
    int index = add_int_ordered(toAdd, quege, 2);//add my req to list
    forEachElementEntered(TAG, quege, quegeCap);
}

void requestWash() {
    pthread_mutex_lock(&slipLock);
    int local = washToRequest;
    washToRequest = 0;
    pthread_mutex_unlock(&slipLock);

    for (int i=0; i<local; i++){
        BUSY = FREE;
        myDirtySlipCount++; 
        broadcastLamport_REQ(washQuege, washQuegeCapacity, WASH_REQ);
    }
}

void putCleanButUsedPantiesBack() {
    pthread_mutex_lock(&washLock);
    myCleanSlips++;
    pthread_mutex_unlock(&washLock);
}

void *slipDisinfect_Thread() {
    sleep(WASH_TIME);
    putCleanButUsedPantiesBack();
}

void putDirtyNastySlipsToWash() {
    myDirtySlipCount--;
    styledPrintln(STYLE_CUSTOM, pid, LAMPORT_CLOCK, "Cleaning slips with love and passion...");
    // task
    pthread_t tid2;
    pthread_create(&tid2, NULL, slipDisinfect_Thread, NULL);
    pthread_detach(tid2);
}

void takeOffYoutPanties() {
    pthread_mutex_lock(&slipLock);
    washToRequest++; // mutex
    pthread_mutex_unlock(&slipLock);
}

void *moonwalkingWithSLipsOnly_Thread() {
    sleep(TASK_TIME);
    takeOffYoutPanties();
}

void wearFreshSlips() {
    BUSY = HAVE_SLIPS;
    styledPrintln(STYLE_CUSTOM, pid, LAMPORT_CLOCK, "Going into the city...");
    // task
    pthread_t tid1;
    pthread_create(&tid1, NULL, moonwalkingWithSLipsOnly_Thread, NULL);
    pthread_detach(tid1);
}

void forEachElementEntered(int TAG, int_array *quege, int quegeCap){

    int minACK = getMinimumACK(quege, quegeCap);
    int myIndex = myNextLamportWentIn(0, quege, quegeCap, minACK);
    
    if(myIndex >= 0) {
        int *elem = get_int(myIndex, quege);
        while (myIndex >= 0){
            // FOR EACH ELEM ENTERED

            switch(TAG){
                case SLIP_REL:
                case SLIP_ACK:
                case SLIP_REQ:
                    printSlip(STYLE_SLIP, pid, LAMPORT_CLOCK, P_OBTAINING, elem[0], pid);
                    wearFreshSlips();
                    break;
                case WASH_REL:
                case WASH_ACK:
                case WASH_REQ:
                    printWash(STYLE_WASH, pid, LAMPORT_CLOCK, P_OBTAINING, elem[0], pid);
                    putDirtyNastySlipsToWash();
                    break;
            }

            // NEXT LOOP if needed
            myIndex = myNextLamportWentIn(myIndex+1, quege, quegeCap, minACK);
        }
    }
}

void acceptTask(Task task) {
    ongoingTask = task;
    BUSY = NEED_SLIPS;
    broadcastLamport_REQ(slipQuege, slipQuegeCapacity, SLIP_REQ);
}

void broadcastLamport_REL(int_array *quege, int quegeCap, int TAG, int index) {
    int *elem = get_int(index, quege);
    int send_buffer[2] = {++LAMPORT_CLOCK, elem[0]};
    switch(TAG) {
        case SLIP_REL:
            printSlip(STYLE_SLIP, pid, LAMPORT_CLOCK, P_RELEASING, elem[0], elem[1]);
            break;
        case WASH_REL:
            printWash(STYLE_WASH, pid, LAMPORT_CLOCK, P_RELEASING, elem[0], elem[1]);
            break;
    }
    broadcast(send_buffer, 0, 2, TAG);

    int lamportOut;
    int index1 = find_my_lamport(0, quege, &lamportOut);// send ACK to all between my requests 1 and 2 in order
    int index2 = find_my_lamport(1, quege, &lamportOut);
    for(int i=index1+1; i<index2; i++){
        int *elem = get_int(i, quege);
        switch(TAG){
            case SLIP_REL:
                sendLamport_ACK(elem[1], elem[0], SLIP_ACK);
                break;
            case WASH_REL:
                sendLamport_ACK(elem[1], elem[0], WASH_ACK);
                break;
        }
    }
    remove_int_ordered(index, quege);
    forEachElementEntered(TAG, quege, quegeCap);
}

void releasePanties() {
    pthread_mutex_lock(&washLock);
    int local = myCleanSlips;
    myCleanSlips = 0;
    pthread_mutex_unlock(&washLock);

    for (int i=0; i<local; i++) {

        int lamport1;
        int index1 = find_my_lamport(0, washQuege, &lamport1);
        if(index1 >= 0) {
            broadcastLamport_REL(washQuege, washQuegeCapacity, WASH_REL, index1);

        }
        
        int lamport2;
        int index2 = find_my_lamport(0, slipQuege, &lamport1);
        if(index2 >= 0){
            broadcastLamport_REL(slipQuege, slipQuegeCapacity, SLIP_REL, index2);
        }
        
        styledPrintln(STYLE_CUSTOM, pid, LAMPORT_CLOCK, "Putting clean panties back...");
    }
}

void dealWithLamport_ACK(int *recvBuffer, MPI_Status *recvStatus, int_array *quege, int quegeCap) {
    printCommRecv(pid, LAMPORT_CLOCK, P_ACK, P_SLIP, recvStatus->MPI_SOURCE);
    int lamport_out;
    int index = find_my_lamport_by_value(recvBuffer[1], quege, &lamport_out); 
    if(index >= 0){ // find REQ and increase its ACK counter
        get_int(index, quege)[2]++;
        forEachElementEntered(recvStatus->MPI_TAG, quege, quegeCap);
    }

}

void sendACKbyTAG(int source, int lamport, int TAG) {
    switch(TAG){
        case SLIP_REQ:
            sendLamport_ACK(source, lamport, SLIP_ACK);
            break;
        case WASH_REQ:
            sendLamport_ACK(source, lamport, WASH_ACK);
            break;
    }
} 

void dealWithLamport_REQ(int *recvBuffer, MPI_Status *recvStatus, int_array *quege, int quegeCap) {
    printCommRecv(pid, LAMPORT_CLOCK, P_REQ, P_SLIP, recvStatus->MPI_SOURCE);
    int buffer[3] = {recvBuffer[0], recvStatus->MPI_SOURCE, 0}; // source lamport, source PID, ACK counter
    int index = add_int_ordered(buffer, quege, 2);
    int lamportOut;
    int myFirst = find_my_lamport(0, quege, &lamportOut);
    
    if( index <= myFirst && index >= 0 || myFirst == -1 ) { //if it went before my first REQ
        sendACKbyTAG(recvStatus->MPI_SOURCE, recvBuffer[0], recvStatus->MPI_TAG);
    }
}


void dealWithLamport_REL(int *recvBuffer, MPI_Status *recvStatus, int_array *quege, int quegeCap) {
    switch(recvStatus->MPI_TAG){
        case SLIP_REL:
            printCommRecv(pid, LAMPORT_CLOCK, P_REL, P_SLIP, recvStatus->MPI_SOURCE);
            break;
        case WASH_REL:
            printCommRecv(pid, LAMPORT_CLOCK, P_REL, P_WASH, recvStatus->MPI_SOURCE);
            break;
    }
    int index = find_any_lamport_by_value(recvBuffer[1], recvStatus->MPI_SOURCE, quege); 
    if(index != -1){ // search for REQ and remove it; check if my occurences are inside critical section
        remove_int_ordered(index, quege);
        forEachElementEntered(recvStatus->MPI_TAG, quege, quegeCap);
    }
}



// for each freeSLips and cleanWash counter, do release



// ---------- CONAN RICARD ARGWALLA ---------- //



void broadcastRELEASE(Task *task) {
    int sendBuffer[3] = {++LAMPORT_CLOCK, task->libID, task->taskID}; // lamport, libID, taskID
    printTask(STYLE_TASK, pid, LAMPORT_CLOCK, P_RELEASING, task->libID, task->taskID);
    // TODO for each client in group
    for(int i=0; i<task->poolSize; i++){
        if(task->participants[i] != pid){
            printCommSend(pid, LAMPORT_CLOCK, P_REL, P_TASK, i);
            MPI_Bsend(sendBuffer, 3, MPI_INT, task->participants[i], TASK_REL, MPI_COMM_WORLD);
        }
    }
}

void sendACKRawParams(int target, int libID, int taskID, int poolSize) {
    printCommSend(pid, LAMPORT_CLOCK, P_ACK, P_TASK, target);

    int sendBuffer[4] = {++LAMPORT_CLOCK, libID, taskID, poolSize}; // lamport, libID, taskID
    MPI_Bsend(sendBuffer, 4, MPI_INT, target, TASK_ACK, MPI_COMM_WORLD);
}
void sendACK(int target, Task *task) {
    sendACKRawParams(target, task->libID, task->taskID, task->poolSize);
}

void broadcastREQ(Task *task){
    task->REQ_Lamport = ++LAMPORT_CLOCK;
    int sendBuffer[4] = {LAMPORT_CLOCK, task->libID, task->taskID, task->poolSize}; // lamport, libID, taskID

    printTask(STYLE_TASK, pid, LAMPORT_CLOCK, P_REQUESTING, task->libID, task->taskID);
    // TODO for each client in group
    for(int i=0; i<task->poolSize; i++){
        if(task->participants[i] != pid){
            printCommSend(pid, LAMPORT_CLOCK, P_REQ, P_TASK, task->participants[i]);
            MPI_Bsend(sendBuffer, 4, MPI_INT, task->participants[i], TASK_REQ, MPI_COMM_WORLD);
        }
    }

    task->isDone = 1;
}

void performPendingACK(Task *task) {
    for(int i=0; i<task->pending_ACK_count; i++){
        sendACK(task->pending_ACK[i], task);
    }
    task->pending_ACK_count = 0;
}

void dealWithTASK(int *recvBuffer, MPI_Status *recvStatus) {
    printCommRecv(pid, LAMPORT_CLOCK, P_TASK, P_TASK, recvStatus->MPI_SOURCE);

    Task newTask = getTask(recvBuffer[1], recvBuffer[2], recvBuffer[3]);
    putParticipantList(&newTask, &recvBuffer[4], recvBuffer[3]);
    update_librarian_max_taskID(newTask.libID, newTask.taskID);
    if(newTask.poolSize == 1) {
        addElem(newTask, onlyMyTasks);
    } else {
        addElem(newTask, taskSet);
    }
}

void dealWithREQ(int *recvBuffer, MPI_Status *recvStatus) {
    printCommRecv(pid, LAMPORT_CLOCK, P_REQ, P_TASK, recvStatus->MPI_SOURCE);
    int foundIndex = -1;
    Task *task;
    task = findTaskByID(recvBuffer[1], recvBuffer[2], true, &foundIndex); // check if task is pending
    
    if(foundIndex == -1){ //if not found
        int *maxID = get_librarian_max_taskID(recvBuffer[1], recvBuffer[2]);
        if( *maxID < recvBuffer[2]){ // if above task max ID
            sendACKRawParams(recvStatus->MPI_SOURCE, recvBuffer[1], recvBuffer[2], recvBuffer[3]);
        } // else it is already done

    } else if (foundIndex >= 0) { // if found and not finished
        
        if(task->REQ_Lamport >= 0 && comparePriority(pid, task->REQ_Lamport, recvStatus->MPI_SOURCE, recvBuffer[0])){ // if there is REQ from me and I am first
            task->pending_ACK[task->pending_ACK_count++] = recvStatus->MPI_SOURCE; // .. so save that ACK for later
        } else { // I am first! But save that pid, if I resign he will get ACK;
            sendACK(recvStatus->MPI_SOURCE, task);
        }
    }
    return;
    
}

void dealWithACK(int *recvBuffer, MPI_Status *recvStatus) {
    printCommRecv(pid, LAMPORT_CLOCK, P_ACK, P_TASK, recvStatus->MPI_SOURCE);

    int foundIndex = -1;
    Task *task =  findTaskByID(recvBuffer[1], recvBuffer[2], true, &foundIndex); // check if task is pending

    if(foundIndex == -1){ //if not
        //ignore it SHOULD NEVER HAPPEN?
    } else if (foundIndex >= 0){ // else if it is pending
        //count ACK's; if you have full set, the task is yours
        task->ACK_count++;
        if(task->ACK_count >= task->poolSize-1){ // check for all ACK - group size - librarian and self
            if(!BUSY){
                printTask(STYLE_TASK, pid, LAMPORT_CLOCK, P_OBTAINING, task->libID, task->taskID);
                broadcastRELEASE(task);
                acceptTask(*task);
                removeElem(foundIndex, taskSet);
            } else { // drop your turn
                printCustomDebug(pid, LAMPORT_CLOCK, "TASK SKIPPED");
                performPendingACK(task);
                task->ACK_count = 0; // reset task
                task->REQ_Lamport= -1; // mark it as new
            }
        }
    }
}

void dealWithRELEASE(int *recvBuffer, MPI_Status *recvStatus) {
    printCommRecv(pid, LAMPORT_CLOCK, P_REL, P_TASK, recvStatus->MPI_SOURCE);

    int foundIndex = -1;
    Task *task = findTaskByID(recvBuffer[1], recvBuffer[2], false, &foundIndex); // check if task is on the list

    if(foundIndex == -1){ //if not
        //ignore it
    } else if (foundIndex >= 0){ // else if it is pending
        printCustomDebug(pid, LAMPORT_CLOCK, "TASK REMOVED");
        removeElem(foundIndex, taskSet);
    }
}



// ----------CONAN MAIN---------- //



void dealWithMessage(int *recvBuffer, MPI_Status *recvStatus) {
    switch (recvStatus->MPI_TAG) {
        case TASK_NEW:
            dealWithTASK(recvBuffer, recvStatus);
            break;
        case TASK_REQ:
            dealWithREQ(recvBuffer, recvStatus);
            break;
        case TASK_ACK:
            dealWithACK(recvBuffer, recvStatus);
            break;
        case TASK_REL:
            dealWithRELEASE(recvBuffer, recvStatus);
            break;
        case SLIP_ACK:
            dealWithLamport_ACK(recvBuffer, recvStatus, slipQuege, slipQuegeCapacity);
            break;
        case SLIP_REQ:
            dealWithLamport_REQ(recvBuffer, recvStatus, slipQuege, slipQuegeCapacity);
            break;
        case SLIP_REL:
            dealWithLamport_REL(recvBuffer, recvStatus, slipQuege, slipQuegeCapacity);
            break;
        case WASH_ACK:
            dealWithLamport_ACK(recvBuffer, recvStatus, washQuege, washQuegeCapacity);
            break;
        case WASH_REQ:
            dealWithLamport_REQ(recvBuffer, recvStatus, washQuege, washQuegeCapacity);
            break;
        case WASH_REL:
            dealWithLamport_REL(recvBuffer, recvStatus, washQuege, washQuegeCapacity);
            break;
    }
}

void runConan_Loop() {

    // init mutex
    pthread_mutex_init(&slipLock, NULL);
    pthread_mutex_init(&washLock, NULL);

    // buffer
    int messageMaxSize = 5 + conanCount;
    int messageMaxSizeByte = messageMaxSize * sizeof(int) + MPI_BSEND_OVERHEAD;
    int bufSizeByte = messageMaxSizeByte * conanCount;

    int *buf = malloc(bufSizeByte);
    MPI_Buffer_attach(buf, bufSizeByte);

    ROLE = CONAN;

    taskSet = createTaskArray(MAX_TASK_QUEGE_SIZE);
    libMaxTaskID = create_int_array(libCount, 2);
    int c = libCount*2;
    int libC = 0;
    for(int i=0; i<c; i+=2){
        libMaxTaskID->array[i] = libC++;
        libMaxTaskID->array[i+1] = 0;
    }
    slipQuege = create_int_array(MAX_LAMPORT_QUEGE_SIZE, 3);
    washQuege = create_int_array(MAX_LAMPORT_QUEGE_SIZE, 3);

    onlyMyTasks = createTaskArray(MAX_TASK_QUEGE_SIZE);

    mainReceiver = createReceiver(messageMaxSize, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD);


    while(true){

        if(!BUSY){
            if(onlyMyTasks->top > 0){ // if have exclusive tasks
                
                printTask(STYLE_TASK, pid, LAMPORT_CLOCK, P_OBTAINING, onlyMyTasks->array[0].libID, onlyMyTasks->array[0].taskID);
                acceptTask(onlyMyTasks->array[0]);
                removeElem(0, onlyMyTasks);

            } else { // REQ new tasks
                for (int i=0; i<taskSet->top; i++){ //REQ all new tasks if not busy
                    if(taskSet->array[i].isDone == 0){
                        broadcastREQ(&taskSet->array[i]);
                    }
                }   
            }
        }
        
        // receive messages
        MPI_Status status;
        int recvFlag = 1;
        int recvMaxCounter = 10;
        while(recvFlag && recvMaxCounter-- >= 0){ 
            try_to_get_message(&mainReceiver, &recvFlag, &status);
            if(recvFlag){
                
                updateLamportClock(*((int*)mainReceiver.buf));
                dealWithMessage(mainReceiver.buf, &status);
                start_new_receive(&mainReceiver);
            }
        }

        requestWash();
        releasePanties();

    }

    destroyReceiver(&mainReceiver);

    drop_int_array(slipQuege);
    drop_int_array(washQuege);
    dropTaskArray(onlyMyTasks);
    dropTaskArray(taskSet);
    drop_int_array(libMaxTaskID);

    MPI_Buffer_detach(buf, &bufSizeByte);
    free(buf);

    pthread_mutex_destroy(&washLock);
    pthread_mutex_destroy(&slipLock);
}



// ---------- MAIN FUNCTIONS ---------- //



/*
 * initialize a single function into mpi with given parameters
 */
MPI_Datatype initMpiStruct(int count, MPI_Datatype datatypes[], int blockLen[], MPI_Aint displacements[]) {

    MPI_Datatype tmpType, taskType;

    MPI_Type_create_struct(count, blockLen, displacements, datatypes, &tmpType);
    MPI_Type_create_resized(tmpType, displacements[0], sizeof(Task), &taskType);

    MPI_Type_free(&tmpType);
    MPI_Type_commit(&taskType);

    return taskType;
}

/* 
 * initialize structures into MPI to be able to send them
 */
void initAllMpiStructs() {

    MPI_TASK_STRUCT = initMpiStruct(
        1,
        (MPI_Datatype[]) {MPI_INT},
        (int[]) {3},
        (MPI_Aint[]) {0} //offsetof(Task, msg_Lamport)
    );

}

int main(int argc, char **argv) {

    //programm argumetns
    libCount = 2;
    slipQuegeCapacity = 3;
    washQuegeCapacity = 2;
    TASK_TIME = 8;
    WASH_TIME = 4;
    LIB_TIME = 10;

    // init process global variables
    LAMPORT_CLOCK = 0;
    BUSY = FREE;
    myDirtySlipCount = 0;
    // init MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &pcount);
	MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    conanCount = pcount - libCount;

    initAllMpiStructs();

    if(pid < libCount){
        runLibrarian_Loop();
    } else {
        runConan_Loop();
    }

	MPI_Finalize();
}