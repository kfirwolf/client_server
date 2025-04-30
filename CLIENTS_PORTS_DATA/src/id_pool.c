#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "id_pool.h"
#include "common_defs.h"


struct id_pool_t {
    char  *in_use;
    size_t capacity;
    size_t top;
    uint32_t id_start_offset;
    uint32_t free_ids[];
};

int id_pool_create(id_pool_t **pool, id_pool_cfg_t *id_pool_cfg) {

    if (unlikely(pool == NULL || id_pool_cfg == NULL || id_pool_cfg->capacity == 0)) {
        return -EINVAL;
    }

    size_t capacity = id_pool_cfg->capacity;
    size_t sz_hdr    = sizeof(id_pool_t);
    size_t sz_free   = capacity * sizeof(uint32_t);
    size_t sz_inuse  = capacity * sizeof(char);
    size_t total     = sz_hdr + sz_free + sz_inuse;

    id_pool_t *p = malloc(total);
    if (unlikely(p == NULL)) {
        return -ENOMEM;
    }

    char *base      = (char *)p;
    p->in_use       = base + sz_hdr + sz_free;
    /* free_ids already refers to base + sz_hdr */
    
    p->capacity = capacity;
    p->top = capacity;
    p->id_start_offset = id_pool_cfg->id_start_offset;
    
    // clear flags
    memset(p->in_use, 0, sz_inuse);
 
    // init free_id array
    for (size_t i = 0; i < p->capacity; i++) {
        p->free_ids[i] = p->id_start_offset + (capacity - 1 - i);
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

    free(pool);

    return 0;
}