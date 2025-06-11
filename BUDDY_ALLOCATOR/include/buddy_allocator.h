#ifndef BUDDY_ALLOCATOR_H
#define BUDDY_ALLOCATOR_H

#include <stdio.h>

typedef struct {
    uint32_t pool_size;
    uint32_t min_block_size;    
} buddy_alloc_cfg_t;

typedef struct {
    uint32_t offset;
    uint32_t size;
} block_t;

typedef struct buddy_allocator buddy_allocator_t;

int buddy_alloc_create(buddy_allocator_t **b_alloc, buddy_alloc_cfg_t *b_cfg);
int buddy_alloc_destroy(buddy_allocator_t *b_alloc);
int buddy_alloc_allocate(buddy_allocator_t *b_alloc, uint32_t block_size, block_t *block_out);
int buddy_alloc_free(buddy_allocator_t *b_alloc, const block_t *block);



#endif // BUDDY_ALLOCATOR_H
