#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "id_pool.h"
#include "common_defs.h"


struct id_pool_t {
    uint32_t *free_ids;
    uint8_t  *in_use;
    uint32_t capacity;
    uint32_t top;
    uint32_t id_start_offset;
};

int id_pool_create(id_pool_t **pool, id_pool_cfg_t *id_pool_cfg) {

    id_pool_t *p;

    if (unlikely(pool == NULL || id_pool_cfg->capacity == 0 )) {
        return -EINVAL;
    }

    p = (id_pool_t *)malloc(sizeof(id_pool_t));
    if (unlikely(p == NULL)) {
        return -EINVAL;
    }

    p->capacity = id_pool_cfg->capacity;
    p->top = id_pool_cfg->capacity;
    p->id_start_offset = id_pool_cfg->id_start_offset;
    p->free_ids = malloc(sizeof(uint32_t)*p->capacity);
    if (unlikely(p->free_ids == NULL)) {
        free(p);
        return -EINVAL;
    }

    p->in_use = calloc(p->capacity, sizeof(uint8_t));
    if (unlikely(p->in_use == NULL)) {
        free(p->free_ids);
        free(p);
        return -EINVAL;
    }

    for (size_t i = 0; i < p->capacity; i++) {
        p->free_ids[i] = p->capacity + p->id_start_offset -i -1;
    }

    *pool = p;

    return 0;
}

int id_pool_allocate(id_pool_t *pool, uint32_t *out_id) {

    if (unlikely(pool == NULL)) {
        return -EINVAL;
    }

    if (pool->top == 0) {
        return -ENOENT;
    }

    *out_id = pool->free_ids[--pool->top];
    uint32_t idx = *out_id - pool->id_start_offset;
    pool->in_use[idx] = 1;
    return 0;
}

int id_pool_free(id_pool_t *pool, uint32_t id) {

    if (unlikely(pool == NULL)) {
        return -EINVAL;
    }

    uint32_t idx = id - pool->id_start_offset;

    //illegal id
    if (unlikely((id > pool->capacity + pool->id_start_offset) || (id < pool->id_start_offset))) {
        return -ESPIPE;
    }

    if (unlikely(!pool->in_use[idx])) {
        // double free detected
        return -EALREADY;
    }

    //free id
    pool->in_use[idx] = 0;
    pool->free_ids[pool->top++] = id;

    return 0;
}

int id_pool_destroy(id_pool_t *pool) {

    if (unlikely(pool == NULL || pool->free_ids == NULL)) {
        return -EINVAL;
    }

    free(pool->free_ids);
    free(pool->in_use);
    free(pool);

    return 0;
}