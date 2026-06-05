#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <common_defs.h>
#include <infra_log.h>
#include "id_pool.h"


struct id_pool_t {
    uint8_t  *in_use;
    uint32_t capacity;
    uint32_t top;
    uint32_t id_start_offset;
    uint32_t free_ids[];
};

int id_pool_create(id_pool_t **pool, id_pool_cfg_t *id_pool_cfg) {

    if (unlikely(pool == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "pool is not initialized");
        return -EINVAL;
    }

    if (unlikely(id_pool_cfg == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "pool cfg is not initialized");
        return -EINVAL;
    }

    if (unlikely(id_pool_cfg->capacity == 0)) {
        NET_INFRA_LOG(LOG_ERROR, "capacity == 0");
        return -EINVAL;
    }

    uint32_t capacity = id_pool_cfg->capacity;
    uint32_t sz_hdr    = sizeof(id_pool_t);
    uint32_t sz_free   = capacity * sizeof(uint32_t);
    uint32_t sz_inuse  = capacity * sizeof(char);
    uint32_t total     = sz_hdr + sz_free + sz_inuse;

    id_pool_t *p = malloc(total);
    if (unlikely(p == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "Memory allocation failed");
        return -ENOMEM;
    }

    uint8_t *base = (uint8_t *)p;
    p->in_use = base + sz_hdr + sz_free;
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
        NET_INFRA_LOG(LOG_ERROR, "pool is not initialized");
        return -EINVAL;
    }

    if (pool->top == 0) {
        NET_INFRA_LOG(LOG_ERROR, "pool is empty");
        return -ENOENT;
    }

    *out_id = pool->free_ids[--pool->top];
    uint32_t idx = *out_id - pool->id_start_offset;
    pool->in_use[idx] = 1;
    return 0;
}

int id_pool_free(id_pool_t *pool, uint32_t id) {

    if (unlikely(pool == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "pool is not initialized");
        return -EINVAL;
    }

    uint32_t idx = id - pool->id_start_offset;

    //illegal id
    if (unlikely((id >= pool->capacity + pool->id_start_offset) || (id < pool->id_start_offset))) {
        NET_INFRA_LOG(LOG_ERROR, "pool illegal id");
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
        NET_INFRA_LOG(LOG_ERROR, "pool is not initialized");
        return -EINVAL;
    }

    free(pool);

    return 0;
}

int id_pool_get_used_id_count(id_pool_t *pool, uint32_t *id_count) {

    if (unlikely(pool == NULL || pool->free_ids == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "pool is not initialized");
        return -EINVAL;
    }

    int count = pool->capacity - pool->top;

    if (count < 0) {
        NET_INFRA_LOG(LOG_ERROR, "ilegal top value");
        return -EINVAL;
    }
    
    *id_count = count;
    return 0;
}