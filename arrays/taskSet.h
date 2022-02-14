#ifndef TASK_SET_H_
#define TASK_SET_H_

#include <stdbool.h>
#include <stdio.h>

typedef struct Task {
    int libID;
    int taskID; 
    int REQ_Lamport;
    int poolSize;
    int ACK_count;
    int* pending_ACK;
    int pending_ACK_count;
    int isDone; // 0 = no, 1 = req, 2 = yes
    int* participants;
} Task;

typedef struct TaskSet {
    int len;
    int top;
    Task* array;
} TaskSet;

//void printArray(TaskSet *task);

TaskSet *createTaskArray(int len);
int addElem(Task elem, TaskSet* set);
int removeElemLeakign(int index, TaskSet *set);
int removeElem(int index, TaskSet *set);
void dropTaskArray(TaskSet *set);

Task *findTask(int libID, int taskID, TaskSet *taskSet, int *outIndex);

Task getTask(int libID, int taskID, int poolSize);
void putParticipantList(Task *task, int *arr, int arrSize);
void dropTask(Task *task);

#endif