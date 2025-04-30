#ifndef ID_POOL_H
#define ID_POOL_H

#include <stdint.h>

typedef struct id_pool_t id_pool_t; //for compilation and proper module design

typedef struct {
    uint32_t capacity;
    uint32_t id_offset;
} id_pool_cfg_t;


int id_pool_create(id_pool_t **pool, id_pool_cfg_t *id_pool_cfg);
int id_pool_allocate(id_pool_t *pool);
int id_pool_free(id_pool_t *pool, uint32_t id);
int id_pool_destroy(id_pool_t *pool);

#endif /* ID_POOL_H */