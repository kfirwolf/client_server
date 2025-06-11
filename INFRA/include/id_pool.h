#ifndef ID_POOL_H
#define ID_POOL_H

#include <stdint.h>

typedef struct id_pool_t id_pool_t; //for compilation and proper module design

/**
 * @brief configure the id pool for init
 *
 * Detailed description (optional).
 */

typedef struct {
    uint32_t capacity; /**< the total amount of the avilable id's in the pool */
    uint32_t id_start_offset; /**< the first id value to start from */
} id_pool_cfg_t;


int id_pool_create(id_pool_t **pool, id_pool_cfg_t *id_pool_cfg);
int id_pool_allocate(id_pool_t *pool, uint32_t *out_id);
int id_pool_free(id_pool_t *pool, uint32_t id);
int id_pool_destroy(id_pool_t *pool);
int id_pool_get_used_id_count(id_pool_t *pool, uint32_t *id_count);

#endif /* ID_POOL_H */