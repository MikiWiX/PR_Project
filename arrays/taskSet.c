#include <stdlib.h>
#include <string.h>

#include "taskSet.h"

// void printArray(TaskSet *set) {
//     char strOut[1000];
//     // special case
//     if(set->top == 0){
//         printf("ARRAY ::: []\n");
//         return;
//     }
    
//     // main loop, without last iteration
//     sprintf(strOut, "[ ");
//     int savedChar = 2;
//     char numberBuffer[100];
    
//     for (int i=0; i<set->top-1; i++){
//         sprintf(numberBuffer, "[%i, %i], ", set->array[i].libID, set->array[i].taskID); // read number to string
//         int numberLen = strlen(numberBuffer);
//         memcpy(&strOut[savedChar], numberBuffer, numberLen); // append to output
//         savedChar += numberLen; // increment value of chars signed
//     }
//     // last iteration
//     if(set->top > 0){
//         sprintf(&strOut[savedChar], "[%i, %i] ]", set->array[set->top-1].libID, set->array[set->top-1].taskID);

//         printf("ARRAY ::: %s\n", strOut);
//     }
    
// }

TaskSet *createTaskArray(int len) {

    TaskSet *set = malloc(sizeof(TaskSet));

    Task *tab = malloc(sizeof(Task)*len);
    set->array = tab;
    set->top = 0;
    set->len = len;

    return set;
}

int addElem(Task elem, TaskSet *set) {
    if (set->top < set->len) {
        set->array[set->top++] = elem;
        return set->top - 1;
    } else {
        return -1;
    }
}

int removeElemLeakign(int index, TaskSet *set) {
    if (set->top > 0) {
        set->array[index] = set->array[--set->top];
        return 1;
    } else {
        return 0;
    }
}

int removeElem(int index, TaskSet *set) {
    if (set->top > 0) {
        dropTask(&set->array[index]);
        set->array[index] = set->array[--set->top];
        return 1;
    } else {
        return 0;
    }
}

void dropTaskArray(TaskSet *set){
    for(int i=0; i<set->top; i++){
        dropTask(&set->array[i]);
    }
    free(set->array);
    free(set);
}

Task *findTask(int libID, int taskID, TaskSet *taskSet, int *outIndex){
    for(int i=0; i<taskSet->top; i++){ // search through tasks
        if(taskSet->array[i].libID == libID &&
        taskSet->array[i].taskID == taskID) {
            *outIndex = i;
            return &taskSet->array[i];
        }
    }
    *outIndex = -1;
    return 0;
}

Task getTask(int libID, int taskID, int poolSize) {
    Task task;

    task.libID = libID;
    task.taskID = taskID;
    task.poolSize = poolSize;
    task.ACK_count = 0;
    task.REQ_Lamport = -1;
    task.pending_ACK = malloc(poolSize*sizeof(int));
    task.pending_ACK_count = 0;
    task.isDone = false;
    task.participants = malloc(poolSize * sizeof(int));

    return task;
}

void putParticipantList(Task *task, int *arr, int arrSize) {
    memcpy(task->participants, arr, arrSize * sizeof(int));
}

void dropTask(Task *task){
    free(task->pending_ACK);
    free(task->participants);
}