#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

//
struct stack {
    int *stackArray;
    int currentSize;
    int size;
};

static struct stack myStack;

int stackInit(int stackSize) {
    myStack.size = stackSize;
    myStack.stackArray = (int *)malloc(sizeof(int)*stackSize);
    if (myStack.stackArray == NULL) {
        return -1;
    }
    myStack.currentSize = 0;

    return 0;
}

bool stackIsEmpty() {

    if (myStack.currentSize == 0) {
        return true;
    }

    return false;
}

bool stackIsFull() {

    if (myStack.currentSize == myStack.size) {
        return true;
    }

    return false;
}

int stackPush(int data) {

    if (stackIsFull()) {
        return -1;
    }

    if (myStack.stackArray == NULL) {
        return -1;
    }

    myStack.stackArray[myStack.currentSize] = data;
    myStack.currentSize++;

    return 0;
}

int stackPop() {

    if (stackIsEmpty()) {
        return -1;
    }

    if (myStack.stackArray == NULL) {
        return -1;
    }

    myStack.currentSize--;

    return myStack.stackArray[myStack.currentSize];
}

int stackFree() {
    if (myStack.stackArray == NULL) {
        return -1;
    }

    free(myStack.stackArray);
    myStack.stackArray = NULL;
    myStack.size = 0;
    myStack.currentSize = 0;
    return 0;
}
