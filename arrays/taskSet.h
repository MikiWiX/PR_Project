#ifndef TASK_SET_H_
#define TASK_SET_H_

#include <stdbool.h>

typedef struct Task {
    int libID;
    int taskID; 
    int REQ_Lamport;
    int poolSize;
    int ACK_count;
    int* pending_ACK;
    int pending_ACK_count;
    bool isDone;
} Task;

typedef struct TaskSet {
    int len;
    int top;
    Task* array;
} TaskSet;

TaskSet *createTaskArray(int len);
int addElem(Task elem, TaskSet* set);
int removeElemLeakign(int index, TaskSet *set);
int removeElem(int index, TaskSet *set);
void dropTaskArray(TaskSet *set);

Task *findTask(int libID, int taskID, TaskSet *taskSet, int *outIndex);

Task getTask(int libID, int taskID, int poolSize);
void dropTask(Task task);

#endif