#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

struct stack {
    uint16_t numOfItems;
    int stackArray[]; //FAM (c99)
};

struct stack_config {
    uint16_t size;
};

int stack_create(struct stack **st, const struct stack_config *cfg) {
    
    if (cfg == NULL || cfg->size == 0) {
        return -EINVAL;
    }

    if ((st) == NULL) {
        return -EINVAL;
    }

    (*st) = malloc(sizeof(struct stack) + sizeof(int)*cfg->size);

    if ( (*st) == NULL) {
        return -ENOMEM;
    }

    (*st)->numOfItems = 0;

    return 0;
}

static inline bool stackIsEmpty(const struct stack *st) {

    if (st->numOfItems == 0) {
        return true;
    }

    return false;
}

static inline bool stackIsFull(const struct stack *st, const struct stack_config *cfg) {

    if (st->numOfItems == cfg->size) {
        return true;
    }

    return false;
}

int stackPush(struct stack *st, const struct stack_config *cfg, uint16_t data) {

    if (st == NULL) {
        return -EINVAL;
    }

    if (stackIsFull(st, cfg)) {
        return -EINVAL;
    }

    st->stackArray[st->numOfItems] = data;
    st->numOfItems++;

    return 0;
}

int stackPop(struct stack *st, uint16_t *outData) {

    if (st == NULL) {
        return -EINVAL;
    }

    if (outData == NULL) {
        return -EINVAL;
    }

    if (stackIsEmpty(st)) {
        return -EINVAL;
    }

    st->numOfItems--;
    (*outData) = st->stackArray[st->numOfItems];
    return 0;
}

int stackFree(struct stack **st) {

    if (st == NULL || (*st) == NULL) {
        return -EINVAL;
    }

    free(*st);
    (*st) = NULL;
    return 0;
}
