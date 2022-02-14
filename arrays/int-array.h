#ifndef INT_ARRAY_H_
#define INT_ARRAY_H_

#include <stdbool.h>

typedef struct int_array {
    int *array;
    int elemSize;
    int top;
    int len;
} int_array;

void printArray(int_array *set);

int_array *create_int_array(int size, int elemSize);
bool areEqual(int *elem1, int* elem2, int count);
bool isGreater(int *elem1, int *elem2, int count);
void set_element(int index, int *element, int_array *set);
void move_element(int targetIndex, int initialIndex, int_array *set);
int add_int(int *num, int_array *set);
int add_int_ordered(int *num, int_array *set, int orderCount);
int remove_int(int index, int_array *set);
void remove_int_ordered(int index, int_array *set);
int *get_int(int index, int_array *set);
int find_int(int *elem, int_array *set);
void drop_int_array(int_array *set);

#endif
