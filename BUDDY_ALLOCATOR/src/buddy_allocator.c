
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <common_defs.h>
#include <infra_log.h>
#include <id_pool.h>
#include "buddy_allocator.h"

#define MAX_NUM_OF_LEVELS 10


struct block {
    struct block *next;
    uint32_t offset;
    uint32_t size;
};

struct buddy_free_list {
    struct block *head;
};

struct buddy_allocator {
    uint32_t pool_size;
    uint32_t min_block_size;
    uint32_t num_levels;
    struct buddy_free_list *free_list;
    struct block *node_pool;
    id_pool_t *max_nodes_id_pool;
    uint32_t total_max_nodes;        
};

static int calc_num_of_levels(uint32_t pool_size, uint32_t min_block_size, uint32_t *num_of_levels) {
    if (min_block_size == 0) {
        return -EINVAL;
    }
    uint32_t levels = 0;

    for (uint32_t t = pool_size / min_block_size; t > 0; t >>= 1) {
        levels++;
    }

    *num_of_levels = levels;
    return 0;  
}

static int calc_max_nodes(uint32_t num_of_levels, uint32_t *max_total_nodes) {
    if (num_of_levels > 32 || num_of_levels == 0) {
        return -EINVAL;       
    }

    uint32_t max_nodes = 0;
    for (size_t i = 0; i < num_of_levels; i++) {
        max_nodes = max_nodes + (1 << i);
    }

    *max_total_nodes = max_nodes;
    return 0;   
}

int buddy_alloc_create(buddy_allocator_t **b_alloc, buddy_alloc_cfg_t *b_cfg) {
    int rc = 0;
    buddy_allocator_t *b_allocator = NULL;
    id_pool_cfg_t client_pool_cfg;
    uint32_t node_pool_idx = 0;

    if (unlikely(b_alloc == NULL || b_cfg == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }    

    //check pool size > min size and both are power of 2.
    uint32_t pool = b_cfg->pool_size;
    uint32_t minb = b_cfg->min_block_size;     

    if ((minb == 0) || (pool == 0) ||
        (minb > pool) ||
        (pool & (pool - 1)) ||
        (minb & (minb - 1)))
    {
        NET_INFRA_LOG(LOG_ERROR, "pool_size and min_block_size must be powers of two, and min_block_size <= pool_size");
        return -EINVAL;
    }
    
    //calculate num of levels (max divition in 2 of the pool size in bytes up to the min block size or from pool_size/min_block to zero)
    uint32_t levels = 0;
    rc = calc_num_of_levels(pool, minb, &levels);
    if (rc != 0) {
        return rc;
    } 

    //calculate max nodes (worst case scenario where if you keep splitting without coalescing, you'll eventually have the max nodes value) 
    uint32_t max_nodes = 0;
    rc = calc_max_nodes(levels, &max_nodes);
    if (rc != 0) {
        return rc;
    }    

    //calc allocation sizes
    const uint32_t base_size = sizeof(buddy_allocator_t);
    const uint32_t free_list_size = levels * sizeof(struct buddy_free_list);
    const uint32_t max_nodes_size = max_nodes * sizeof(struct block);

    const size_t total_size = base_size + free_list_size + max_nodes_size;

    b_allocator = calloc(1, total_size);

    if (unlikely(b_allocator == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "Memory allocation failed");
        return -ENOMEM;
    }    

    b_allocator->min_block_size = minb;
    b_allocator->pool_size = pool;
    b_allocator->num_levels = levels;
    b_allocator->total_max_nodes = max_nodes;

    b_allocator->free_list = (struct buddy_free_list *)((uint8_t *)b_allocator + base_size);
    b_allocator->node_pool = (struct block *)((uint8_t *)b_allocator + base_size + free_list_size);

    client_pool_cfg.capacity = b_allocator->total_max_nodes;
    client_pool_cfg.id_start_offset = 0;

    rc = id_pool_create(&(b_allocator->max_nodes_id_pool), &client_pool_cfg);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while creating maximum nodes pool.");
        free(b_allocator);
        return rc;
    }

    rc = id_pool_allocate(b_allocator->max_nodes_id_pool, &node_pool_idx);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to allocate new node");
        id_pool_destroy(b_allocator->max_nodes_id_pool); 
        free(b_allocator);      
        return rc;
    }    

    struct block *free_block = &b_allocator->node_pool[node_pool_idx];
    free_block->next = NULL;
    free_block->offset = 0;
    free_block->size = b_allocator->pool_size;
    // using index 0 here suggest i use the convention block size large to small in the buddy allocator 
    b_allocator->free_list[0].head = free_block;
    *b_alloc = b_allocator;

    return 0;
}

int buddy_alloc_destroy(buddy_allocator_t *b_alloc) {

}

int buddy_alloc_allocate(buddy_allocator_t *b_alloc, uint32_t block_size, block_t *block_out) {

}

int buddy_alloc_free(buddy_allocator_t *b_alloc, const block_t *block) {

}