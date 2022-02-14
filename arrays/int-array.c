#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "int-array.h"

void printArray(int_array *set) {
    char strOut[1000];
    // special case
    if(set->top == 0){
        printf("ARRAY ::: []\n");
        return;
    }
    
    // main loop, without last iteration
    sprintf(strOut, "[ ");
    int savedChar = 2;
    char numberBuffer[100];
    
    for (int i=0; i<set->top-1; i++){
        int realIndex = i * set->elemSize;
        sprintf(numberBuffer, "[%i, %i, %i], ", set->array[realIndex], set->array[realIndex+1], set->array[realIndex+2]); // read number to string
        int numberLen = strlen(numberBuffer);
        memcpy(&strOut[savedChar], numberBuffer, numberLen); // append to output
        savedChar += numberLen; // increment value of chars signed
    }
    // last iteration
    if(set->top > 0){
        int realIndex = (set->top-1) * set->elemSize;
        sprintf(&strOut[savedChar], "[%i, %i, %i] ]", set->array[realIndex], set->array[realIndex+1], set->array[realIndex+2]);

        printf("ARRAY ::: %s\n", strOut);
    }
    
}

int_array *create_int_array(int size, int elemSize) {
    int_array *set = malloc(sizeof(int_array));
    int *tab = malloc(sizeof(int)*elemSize*size);
    set->array = tab;
    set->elemSize = elemSize;
    set->top = 0;
    set->len = size;
    return set;
}

bool areEqual(int *elem1, int* elem2, int count){
    for(int i=0; i<count; i++){
        if(elem1[i] != elem2[i]){
            return false;
        }
    }
    return true;
}

bool isGreater(int *elem1, int *elem2, int count){
    for(int i=0; i<count; i++){
        if(elem1[i] > elem2[i]){
            return true;
        } else if (elem1[i] < elem2[i]) {
            return false;
        }
    }
}

void set_element(int index, int *element, int_array *set){
    int realIndex = index * set->elemSize;
    for (int i=0; i<set->elemSize; i++){
        set->array[realIndex+i] = element[i];
    }
}

void move_element(int targetIndex, int initialIndex, int_array *set){
    int indx1 = initialIndex * set->elemSize;
    int indx2 = targetIndex * set->elemSize;
    for(int i=0; i<set->elemSize; i++){
        set->array[indx2+i] = set->array[indx1+i];
    }
}

int add_int(int *num, int_array *set) {
    if (set->top < set->len) {
        int index = set->elemSize * set->top++;
        for(int i=0; i<set->elemSize; i++){
            set->array[index+i] = num[i];
        }
        return 1;
    } else {
        return 0;
    }
}

int add_int_ordered(int *num, int_array *set, int orderCount) {
    
    if (set->top < set->len) {

        int index = set->top * set->elemSize;
        for(int i=set->top-1; i>=0; i--){ // for each element
            index -= set->elemSize;
            if(!isGreater(num, &set->array[index], orderCount)){ // if new element index is lower
                move_element(i+1, i, set);
            } else {
                set_element(i+1, num, set);
                set->top++;
                
                return i+1;
            }
        }
        set_element(0, num, set); // lastly element 0
        set->top++;
        
        return 0;
    } else {
        
        return -1;
    }
}

int remove_int(int index, int_array *set) {
    if (set->top > 0) {
        move_element(index, --set->top, set);
        return 1;
    } else {
        return 0;
    }
}

void remove_int_ordered(int index, int_array *set) {
    for(int i=index; i<set->top; i++){
        move_element(i, i+1, set);
    }
    set->top--;
}

int *get_int(int index, int_array *set){
    int indxx = index * set->elemSize;
    return &set->array[indxx];
}

int find_int(int *elem, int_array *set) {
    for(int i=0; i<set->top; i++){
        if(areEqual(elem, &set->array[i], set->elemSize)){
            return i;
        }
    }
    return -1;
}

void drop_int_array(int_array *set){
    free(set->array);
    free(set);
}


// --- MODIFY BEFORE USING --- //

// void printArray(int_array *set) {
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
//         int realIndex = i * set->elemSize;
//         sprintf(numberBuffer, "[%i, %i, %i], ", set->array[realIndex], set->array[realIndex+1], set->array[realIndex+2]); // read number to string
//         int numberLen = strlen(numberBuffer);
//         memcpy(&strOut[savedChar], numberBuffer, numberLen); // append to output
//         savedChar += numberLen; // increment value of chars signed
//     }
//     // last iteration
//     if(set->top > 0){
//         int realIndex = (set->top-1) * set->elemSize;
//         sprintf(&strOut[savedChar], "[%i, %i, %i] ]", set->array[realIndex], set->array[realIndex+1], set->array[realIndex+2]);

//         printf("ARRAY ::: %s\n", strOut);
//     }
    
// }
