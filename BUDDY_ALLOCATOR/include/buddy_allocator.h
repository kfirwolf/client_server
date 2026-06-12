#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef struct {
    uint32_t total_mem_pool_size;
    uint32_t min_block_size;    
} buddy_alloc_cfg_t;

typedef struct buddy_alloc buddy_alloc_t;
typedef struct buddy_alloc_block buddy_alloc_block_t;

int buddy_alloc_create(buddy_alloc_cfg_t *b_cfg, buddy_alloc_t **b_alloc);
int buddy_alloc_destroy(buddy_alloc_t *b_alloc);
int buddy_alloc_allocate(buddy_alloc_t *b_alloc, uint32_t block_size, buddy_alloc_block_t **block_out);
int buddy_alloc_free(buddy_alloc_t *b_alloc, buddy_alloc_block_t *block);

int buddy_alloc_get_num_of_levels(const buddy_alloc_t *b_alloc, uint32_t *num_of_levels);
int buddy_alloc_count_free_blocks(const buddy_alloc_t *b_alloc, uint32_t level, uint32_t *free_blocks_num);

// block getter
int buddy_alloc_get_block_level(const buddy_alloc_block_t *blk, uint32_t *blk_level);
int buddy_alloc_get_block_offset(const buddy_alloc_block_t *blk, uint32_t *blk_offset);

#ifdef __cplusplus
}
#endif

#endif // BUDDY_ALLOCATOR_H