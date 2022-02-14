#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "printer.h"

const bool debugPrints = false;

int getCharArraySize(int count) {
    if ( count >= 0 ) {
        return 13*count + 2;
    } else {
        return -1;
    }
}

void intArrayToString(char *strOut, int strSize, int *arr, int elements) {

    // special case
    if(elements == 0){
        strOut = "[]";
        return;
    }
    
    // main loop, without last iteration
    sprintf(strOut, "[ ");
    int savedChar = 2;
    char numberBuffer[13];
    
    for (int i=0; i<elements-1; i++){
        sprintf(numberBuffer, "%i, ", arr[i]); // read number to string
        int numberLen = strlen(numberBuffer);
        memcpy(&strOut[savedChar], numberBuffer, numberLen); // append to output
        savedChar += numberLen; // increment value of chars signed
    }
    // last iteration
    sprintf(&strOut[savedChar], "%i ]", arr[elements-1]); 

}

void printNewTask(char *COLOR, int pid, int lamport, char *OPname, int libID, int taskID, int *targets, int arrSize) { //, int *conanArr, int conanCount

    int strSize = getCharArraySize(arrSize);
    char printedArray[strSize];
    intArrayToString(printedArray, strSize, targets, arrSize);

    printf("\e[1;%sm PID: %i   C: %i\e[22m"
    " --- %s TASK "
    "\e[3m(libID = %i, taskID = %i) %s\e[0m\n", 
    COLOR, pid, lamport, OPname, libID, taskID, printedArray);

}

void printTask(char *COLOR, int pid, int lamport, char *OPname, int libID, int taskID) { //, int *conanArr, int conanCount

    printf("\e[1;%sm PID: %i   C: %i\e[22m"
    " --- %s TASK "
    "\e[3m(libID = %i, taskID = %i)\e[0m\n", 
    COLOR, pid, lamport, OPname, libID, taskID);

}

void printSlip(char *COLOR, int pid, int lamport, char *OPname, int reqLamport, int reqPid) {

    printf("\e[1;%sm PID: %i   C: %i\e[22m"
    " --- %s SLIPS "
    "\e[3m(reqLamport = %i, reqPid = %i)\e[0m\n", 
    COLOR, pid, lamport, OPname, reqLamport, reqPid);

}

void printWash(char *COLOR, int pid, int lamport, char *OPname, int reqLamport, int reqPid) {

    printf("\e[1;%sm PID: %i   C: %i\e[22m"
    " --- %s WASH "
    "\e[3m(reqLamport = %i, reqPid = %i)\e[0m\n", 
    COLOR, pid, lamport, OPname, reqLamport, reqPid);

}

void styledPrintln(char *style, int pid, int lamport, char* message) {
    printf("\e[%sm PID: %i   C: %i --- %s\e[0m\n", style, pid, lamport, message);
}

/// DEBUG PRINTS
#define DEBUG_STYLE "\e[2m"

void printCommSend(int pid, int lamport, char *TYPE, char *OPname, int target) {
    if(debugPrints){
        printf(DEBUG_STYLE "PID: %i   C: %i --- %s (%s) -> pid %i\e[0m\n", pid, lamport, TYPE, OPname, target);
    }
}
void printCommRecv(int pid, int lamport, char *TYPE, char *OPname, int source) {
    if(debugPrints){
        printf(DEBUG_STYLE "PID: %i   C: %i <- %s (%s) --- pid %i\e[0m\n", pid, lamport, TYPE, OPname, source);
    }
}
void printCustomDebug(int pid, int lamport, char *message) {
    if(debugPrints){
        printf(DEBUG_STYLE "PID: %i   C: %i --- %s\e[0m\n", pid, lamport, message);
    }
}