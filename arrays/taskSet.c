#include <stdlib.h>

#include "taskSet.h"

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
        return 1;
    } else {
        return 0;
    }
}

int removeElem(int index, TaskSet *set) {
    if (set->top > 0) {
        dropTask(set->array[index]);
        set->array[index] = set->array[set->top--];
        return 1;
    } else {
        return 0;
    }
}

void dropTaskArray(TaskSet *set){
    for(int i=0; i<set->top; i++){
        dropTask(set->array[i]);
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

    return task;
}

void dropTask(Task task){
    free(task.pending_ACK);
}