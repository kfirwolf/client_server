#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "id_pool.h"


struct id_pool_t {
    uint32_t *free_ids;
    uint8_t  *in_use;
    uint32_t capacity;
    uint32_t top;
    uint32_t id_offset;

};

int id_pool_create(id_pool_t **pool, id_pool_cfg_t *id_pool_cfg) {

    if (pool == NULL || id_pool_cfg->capacity == 0 ) {
        return -EINVAL;
    }

    *pool = (id_pool_t *)malloc(sizeof(id_pool_t));
    if (*pool == NULL) {
        return -EINVAL;
    }

    (*pool)->capacity = id_pool_cfg->capacity;
    (*pool)->top = id_pool_cfg->capacity;
    (*pool)->id_offset = id_pool_cfg->id_offset;
    (*pool)->free_ids = malloc(sizeof(uint32_t)*(*pool)->capacity);
    (*pool)->in_use = calloc((*pool)->capacity, sizeof(uint8_t));

    if ((*pool)->free_ids == NULL || (*pool)->in_use == NULL) {
        free((*pool)->free_ids);
        free((*pool)->in_use);
        free((*pool));
        return -EINVAL;
    }

    for (size_t i = 0; i < (*pool)->capacity; i++) {
        (*pool)->free_ids[i] = (*pool)->capacity + (*pool)->id_offset -i -1;
    }

    return 0;
}

int id_pool_allocate(id_pool_t *pool) {

    if (pool == NULL) {
        return -EINVAL;
    }

    if (pool->top == 0) {
        return -ENOENT;
    }
    int id = pool->free_ids[--pool->top];
    uint32_t idx = id - pool->id_offset;
    pool->in_use[idx] = 1;
    return (int)id;
}

int id_pool_free(id_pool_t *pool, uint32_t id) {

    if (pool == NULL) {
        return -EINVAL;
    }

    uint32_t idx = id - pool->id_offset;

    //illegal id
    if ((id > pool->capacity + pool->id_offset) || (id < pool->id_offset)) {
        return -ESPIPE;
    }

    if (!pool->in_use[idx]) {
        // double free detected
        return -EALREADY;
    }

    //free id
    pool->in_use[idx] = 0;
    pool->free_ids[pool->top++] = id;

    return 0;
}

int id_pool_destroy(id_pool_t *pool) {

    if (pool == NULL || pool->free_ids == NULL) {
        return -EINVAL;
    }

    free(pool->free_ids);
    free(pool->in_use);
    free(pool);

    return 0;
}