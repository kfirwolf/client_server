#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

struct stack {
    uint16_t num_Of_items;
    int stack_array[]; //FAM (c99)
};

struct stack_config {
    uint16_t size;
};

int stack_create(struct stack **st, const struct stack_config *cfg) {
    
    if (cfg == NULL || cfg->size == 0) {
        return -EINVAL;
    }

    if (st == NULL) {
        return -EINVAL;
    }

    (*st) = malloc(sizeof(struct stack) + sizeof(int)*cfg->size);

    if (*st == NULL) {
        return -ENOMEM;
    }

    memset(*st, 0, sizeof(struct stack) + sizeof(int)*cfg->size); //can also have used calloc instead

    return 0;
}

static inline bool stackIsEmpty(const struct stack *st) {
    return !(st->num_Of_items);
}

static inline bool stackIsFull(const struct stack *st, const struct stack_config *cfg) {
    return (st->num_Of_items == cfg->size);
}

int stackPush(struct stack *st, const struct stack_config *cfg, uint16_t data) {

    if (st == NULL) {
        return -EINVAL;
    }

    if (stackIsFull(st, cfg)) {
        return -EINVAL;
    }

    st->stack_array[st->num_Of_items] = data;
    st->num_Of_items++;

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

    st->num_Of_items--;
    (*outData) = st->stack_array[st->num_Of_items];
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

