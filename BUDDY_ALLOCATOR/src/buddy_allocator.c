
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <common_defs.h>
#include <infra_log.h>
#include <id_pool.h>
#include "buddy_allocator.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define MAX_LEVELS 64

typedef struct {
    void *elements_array;
    uint32_t num_of_elements;
    uint32_t size_of_element;
} mem_pool_t;

typedef struct {
    uint32_t element_size; 
    uint32_t num_elements;
} mem_pool_cfg_t;

struct buddy_alloc_block {
    struct buddy_alloc_block *next; 
    struct buddy_alloc_block *prev; // we can add prev pointer to the block struct to make the linked list operations more efficient (removing a block from the middle of the list will be O(1) instead of O(n) with only next pointer)
    uint32_t offset; // the offset of the block in the memory pool (relative to the start of the pool), used to calculate buddy offset and block size
    uint32_t level : 6; // block's level in the buddy tree, used to derive block size and slot index
    uint32_t is_active : 1; // true if this slot contains a valid block object, false if the slot is empty (e.g. after a merge)
    uint32_t is_allocated : 1; // true if the block is currently allocated to the user, false if it's in the free list.
    uint32_t reserved : 24; // reserved bits for future use (if we want to add more info to the block struct, we can use these bits without the needs to change the struct size and break the API)
};

struct buddy_free_list {
    struct buddy_alloc_block *head;
};

struct buddy_alloc {
    uint32_t min_block_size; 
    uint32_t num_of_levels;
    mem_pool_t *mem_pool;
    struct buddy_free_list free_list[];
};

static inline uint32_t get_buddy_offset(uint32_t block_offset, uint32_t block_size) {

    // Calculate buddy offset: buddy = offset ^ size
    // e.g., 2 ^ 4 = 6 → buddy of 2 is 6; 6 ^ 4 = 2 → buddy of 6 is 2    
    return block_offset ^ block_size;
}

static inline uint32_t get_block_size(uint32_t block_level, uint32_t min_block_size, uint32_t num_of_levels) {
    return min_block_size << (num_of_levels - 1 - block_level);
}

static inline int calc_num_of_levels(uint32_t block_size, uint32_t min_block_size, uint32_t *num_of_levels) {

    if (min_block_size == 0) {
        return -EINVAL;
    }
    uint32_t levels = 0;

    for (uint32_t t = block_size / min_block_size; t > 0; t >>= 1) {
        levels++;
    }

    *num_of_levels = levels;
    return 0;  
}

static inline uint32_t next_pow2(uint32_t v) {
    if (v == 0) return 0;
    v--;               // 1) Decrement by 1
    v |= v >> 1;       // 2) “Spread” highest bit right by 1
    v |= v >> 2;       // 3) Spread right by 2 more
    v |= v >> 4;       // 4) by 4
    v |= v >> 8;       // 5) by 8
    v |= v >> 16;      // 6) by 16
    return v + 1;      // 7) Add 1 to get the next power‑of‑two
}

static inline int calc_max_nodes(uint32_t num_of_levels, uint32_t *max_total_nodes) {
    if (num_of_levels > MAX_LEVELS || num_of_levels == 0) {
        return -EINVAL;       
    }

    uint32_t max_nodes = 0;
    for (size_t i = 0; i < num_of_levels; i++) {
        max_nodes = max_nodes + (1 << i);
    }

    *max_total_nodes = max_nodes;
    return 0;   
}

static int get_block_level(const buddy_alloc_t *b_alloc, uint32_t block_size, uint32_t *level_out)
{
    if (b_alloc == NULL || level_out == NULL) {
        return -EINVAL;
    }
    // Must be a power-of-two multiple of min_block_size
    uint32_t minb = b_alloc->min_block_size;
    if (block_size < minb || (block_size & (block_size - 1)) != 0) {
        NET_INFRA_LOG(LOG_ERROR, "illegal block size value: %u", block_size);
        return -EINVAL;
    }

    uint32_t shift_count = 0;
    uint32_t size = block_size;
    while (size > minb) {
        size >>= 1;
        shift_count++;
    }

    uint32_t max_lvl = b_alloc->num_of_levels - 1;
    if (shift_count > max_lvl) {
        NET_INFRA_LOG(LOG_ERROR, "illegal shift_count value: %u", shift_count);    
        return -EINVAL;
    }
    *level_out = max_lvl - shift_count;
    return 0;
}

static inline uint32_t calc_buddy_allocator_slot(uint32_t offset, uint32_t block_level, uint32_t block_size) {

    return (((1U << block_level) -1U) + (uint32_t)(offset/block_size));  
}

//////////////////////////////// INTERNAL MEM POOL API ///////////////////////////////////////////////////////////////

static int mem_pool_create(mem_pool_t **pool, mem_pool_cfg_t *mem_pool_cfg) {

    if (pool == NULL || mem_pool_cfg  == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "Node pool invalid values");
        return -EINVAL;
    }

    if (mem_pool_cfg->num_elements == 0 || mem_pool_cfg->element_size == 0) {
        NET_INFRA_LOG(LOG_ERROR, "cfg values == 0");
        return -EINVAL;   
    }

    *pool = calloc(1, sizeof(mem_pool_t));
    if (unlikely(*pool == NULL)) {
        return -ENOMEM;
    }

    (*pool)->elements_array = (void *)calloc(mem_pool_cfg->num_elements, mem_pool_cfg->element_size);

    if (unlikely((*pool)->elements_array == NULL)) {
        free(*pool);
        NET_INFRA_LOG(LOG_ERROR, "Node pool memory allocation failed");
        return -ENOMEM;
    }

    (*pool)->num_of_elements = mem_pool_cfg->num_elements;
    (*pool)->size_of_element = mem_pool_cfg->element_size;

    NET_INFRA_LOG(LOG_DEBUG, "Finished mem_pool_create.");
    return 0;
}

static int mem_pool_destroy(mem_pool_t *pool) {

    if (pool == NULL) {
        return -EINVAL;
    }

    free(pool->elements_array);
    free(pool);
    return 0;
}

static int mem_pool_get_block(const mem_pool_t *pool, void **ptr, const uint32_t slot_index) {

    if (pool == NULL || ptr == NULL || slot_index >= pool->num_of_elements) {
        return -EINVAL;
    }

    *ptr = (void *)((char *)pool->elements_array + (slot_index * pool->size_of_element));

    return 0;
}

//////////////////////////////// INTERNAL LINKED LIST API //////////////////////////////////////////////////////////////

static int create_new_block(buddy_alloc_t *b_alloc, uint32_t level, uint32_t offset, uint32_t block_size, struct buddy_alloc_block **out_block) {
    int rc = 0;
    struct buddy_alloc_block *free_block;

    if (out_block == NULL) {
        return -EINVAL;
    }

    uint32_t slot_index = calc_buddy_allocator_slot(offset, level, block_size);

    rc = mem_pool_get_block(b_alloc->mem_pool, (void **)&free_block, slot_index);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to allocate new block");
        return rc;
    }

    if (free_block->is_active) {
        NET_INFRA_LOG(LOG_ERROR, "Slot already active at level %u offset %u", level, offset);
        return -EINVAL;
    }
        
    free_block->level = level;
    free_block->next = NULL;
    free_block->prev = NULL;
    free_block->offset = offset;
    free_block->is_active = true;
    free_block->is_allocated = false;
    *out_block = free_block;
    return 0;
}

static int linked_list_add_block(buddy_alloc_t *b_alloc, struct buddy_alloc_block *input_block) {

    if (unlikely(input_block == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to add block to free list");
        return -EINVAL;
    }

    if (unlikely(b_alloc->free_list[input_block->level].head != NULL)) {
        b_alloc->free_list[input_block->level].head->prev = input_block;
    }
    
    input_block->next = b_alloc->free_list[input_block->level].head;
    input_block->prev = NULL;
    b_alloc->free_list[input_block->level].head = input_block;

    return 0;
}

static int linked_list_extract_block(buddy_alloc_t *b_alloc, struct buddy_alloc_block *target_block, uint32_t level) {

    struct buddy_alloc_block *next_block = target_block->next;
    struct buddy_alloc_block *prev_block = target_block->prev;

    if (next_block != NULL) {
        next_block->prev = prev_block;
    }

    if (prev_block != NULL) {
        prev_block->next = next_block;
    }
    else {
        b_alloc->free_list[level].head = next_block;
    }

    return 0;
}

//////////////////////////////// INTERNAL BUDDY ALLOCATOR API //////////////////////////////////////////////////////////

static int remove_buddy_block(buddy_alloc_t *b_alloc, const uint32_t *offset, int *level, const uint32_t *block_size, uint32_t *buddy_offset, bool *found_buddy) {
    int rc = 0;
    *found_buddy = false;
    *buddy_offset = get_buddy_offset(*offset, *block_size);
    
    uint32_t buddy_slot = calc_buddy_allocator_slot(*buddy_offset, *level, *block_size);
    struct buddy_alloc_block *buddy_block;
    rc = mem_pool_get_block(b_alloc->mem_pool, (void **)&buddy_block, buddy_slot);

    if (unlikely(rc != 0)) {
        return rc;            
    }

    if (!buddy_block->is_active || buddy_block->is_allocated) {
        *found_buddy = false;
        return 0;
    }

    *found_buddy = true;

    //remove buddy block from free linked list and dealocate.
    rc = linked_list_extract_block(b_alloc, buddy_block, *level);
    if (unlikely(rc != 0)) {
        return rc;            
    }

    buddy_block->is_active = false;
    return 0;
}

static int allocate_at_level(buddy_alloc_t *b_alloc, uint32_t level, struct buddy_alloc_block **block_out, bool *found_block) {

    int rc = 0;
    struct buddy_alloc_block* target_block = NULL;
    
    if (block_out == NULL) {
        return -EINVAL;
    }

    target_block = b_alloc->free_list[level].head;
    if (target_block == NULL) {
        *found_block = false;
        return 0;
    }


    //remove block from free list (not from mem pool)
    rc = linked_list_extract_block(b_alloc, target_block, level);
    if (unlikely(rc != 0)) {
        return rc;            
    }

    target_block->is_allocated = true;
    *found_block = true; //passed block pointer to output
    *block_out = target_block;
    return 0;
}

static inline int find_non_empty_level(buddy_alloc_t *b_alloc, uint32_t start_level, uint32_t *found_level) {
    uint32_t level = start_level;

    //search for non empty level worst case of O(num_of_levels) but in practice will be much less since we will find free blocks in the upper levels (larger blocks) and split them to the target level (smaller blocks)
    while (level > 0 && b_alloc->free_list[level].head == NULL) {
        level--;
    }

    if (b_alloc->free_list[level].head == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "No free space for allocating a block");
        return  -ENFILE;     
    }

    *found_level = level;
    return 0;
}

static int split_block_down(buddy_alloc_t *b_alloc, uint32_t src_level, uint32_t target_level, struct buddy_alloc_block **target_blk) {
    int rc = 0;
    struct buddy_alloc_block *start_block;
    struct buddy_alloc_block *buddy = NULL;
    uint32_t blk_offset;
    uint32_t buddy_offset;
    uint32_t b_size;


    if (target_blk == NULL || b_alloc == NULL) {
        return -EINVAL;
    }

    if (src_level >= b_alloc->num_of_levels ||
        target_level >= b_alloc->num_of_levels ||
        target_level <= src_level) {
        return -EINVAL;
    }

    start_block = b_alloc->free_list[src_level].head;
    if (start_block == NULL) {
        return -ENOMEM;
    }

    blk_offset = start_block->offset;    

    // remove the src block (we only remove from the free list , but keep the block object start_block and not returnning it to the mem pool, we will use the same block for the target block at the end)
    rc = linked_list_extract_block(b_alloc, start_block, src_level);
    if (unlikely(rc != 0)) {
        return rc;
    }

    start_block->is_active = false;

    // time complexity: o(target_level - src_level) worst case o(num_of_levels), we split block recursively until we get to the target level, in each split we create a buddy block and add it to the free list
    for (uint32_t lev = src_level + 1; lev <= target_level; ++lev) {

        //creating a buddy block for each level
        b_size = get_block_size(lev, b_alloc->min_block_size, b_alloc->num_of_levels);
        buddy_offset = get_buddy_offset(blk_offset, b_size);
        rc = create_new_block(b_alloc, lev, buddy_offset, b_size, &buddy);
        if (unlikely(rc != 0)) {
            return rc;
        }

        rc = linked_list_add_block(b_alloc, buddy);
        if (unlikely(rc != 0)) {
            return rc;
        }
    }

    rc = create_new_block(b_alloc, target_level, blk_offset, b_size, target_blk);
    if (unlikely(rc != 0)) {
        return rc;
    }    

    return 0;
}

//////////////////////////////// EXTERNAL BUDDY ALLOCATOR API //////////////////////////////////////////////////////////

int buddy_alloc_create(buddy_alloc_cfg_t *b_cfg, buddy_alloc_t **b_alloc) {
    int rc = 0;
    buddy_alloc_t *b_allocator = NULL;
    mem_pool_cfg_t mem_pool_cfg;
    struct buddy_alloc_block *blk;

    if (unlikely(b_alloc == NULL || b_cfg == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }    

    *b_alloc = NULL;

    //check pool size > min size and both are power of 2.
    uint32_t total_mem_pool = b_cfg->total_mem_pool_size;
    uint32_t minb = b_cfg->min_block_size;     

    if ((minb == 0) || (total_mem_pool == 0) || (total_mem_pool & (total_mem_pool - 1)) || (minb & (minb - 1)))
    {
        NET_INFRA_LOG(LOG_ERROR, "pool_size and min_block_size must be powers of two and larger then zero");
        return -EINVAL;
    }

    if ((minb > total_mem_pool)) {
        NET_INFRA_LOG(LOG_ERROR, "min_block_size > total_mem_pool");
        return -EINVAL;        
    }
    
    //calculate num of levels (max divition in 2 of the pool size in bytes up to the min block size or from pool_size/min_block to zero)
    uint32_t num_of_levels = 0;
    rc = calc_num_of_levels(total_mem_pool, minb, &num_of_levels);
    if (rc != 0) {
        return rc;
    }

    if (num_of_levels > MAX_LEVELS) {
        NET_INFRA_LOG(LOG_ERROR, "Number of levels exceeds MAX_LEVELS (%u)", MAX_LEVELS);
        return -EINVAL;
    }

    //calculate max nodes (worst case scenario where if you keep splitting without coalescing, you'll eventually have the max nodes value) 
    uint32_t max_nodes = 0;
    rc = calc_max_nodes(num_of_levels, &max_nodes);
    if (unlikely(rc != 0)) {
        return rc;
    }

    //calc allocation sizes
    const uint32_t base_size = sizeof(buddy_alloc_t);
    const uint32_t free_list_size = num_of_levels * sizeof(struct buddy_free_list);
    const size_t total_size = base_size + free_list_size;
    
    b_allocator = calloc(1, total_size);

    if (unlikely(b_allocator == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "b_allocator memory allocation failed");
        rc = -ENOMEM;
        goto fail;
    }

    //setting and creating mem pool
    mem_pool_cfg.num_elements = max_nodes;
    mem_pool_cfg.element_size = sizeof(struct buddy_alloc_block);

    rc = mem_pool_create(&b_allocator->mem_pool, &mem_pool_cfg);
    if (unlikely(rc != 0)) {
        goto fail;
    }
    
    //setting buddy allocator
    b_allocator->min_block_size = minb;
    b_allocator->num_of_levels = num_of_levels;

    //setting first (largest block) level linked list
    rc = create_new_block(b_allocator, 0, 0, total_mem_pool, &blk);
    if (unlikely(rc != 0)) {
        goto fail;
    }
    
    rc = linked_list_add_block(b_allocator, blk);
    if (unlikely(rc != 0)) {
        goto fail;
    }    

    *b_alloc = b_allocator;

    NET_INFRA_LOG(LOG_INFO, "Buddy allocator successfully created");    
    return 0;

    fail:
        if (b_allocator) {
            if (b_allocator->mem_pool) mem_pool_destroy(b_allocator->mem_pool);
            free(b_allocator);
        }
        return rc;    
}

int buddy_alloc_destroy(buddy_alloc_t *b_alloc) {

    int rc = 0;
    if (unlikely(b_alloc == NULL)) {
        return -EINVAL;
    }

    //Tear down the mem pool
    rc = mem_pool_destroy(b_alloc->mem_pool);

    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_INFO, "mem pool destroy failed");
        free(b_alloc);
        return rc;
    }  

    //Free the block that holds your buddy_allocator_t, free_list[], 
    free(b_alloc);
    NET_INFRA_LOG(LOG_INFO, "Buddy allocator destroyed");

    return 0;
}

int buddy_alloc_allocate(buddy_alloc_t *b_alloc, uint32_t block_size, struct buddy_alloc_block **block_out) {
    int rc;
    uint32_t original_block_level, level, split_size;
    split_size = block_size;
    struct buddy_alloc_block *req_block;

    if (unlikely(b_alloc == NULL || block_out == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }    
    
    if (unlikely(split_size == 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Requested block size == 0");
        return -EINVAL;
    }

    split_size = next_pow2(split_size);

    rc = get_block_level(b_alloc, split_size, &original_block_level);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "illegal block level value: %d", original_block_level);        
        return rc;
    }

    level = original_block_level;
    bool found_block;
    rc = allocate_at_level(b_alloc, level, &req_block, &found_block); //update the inner func that this func uses to not remove the mem block from the mem pool when allocate the block
    if ( rc!= 0) {
        return rc;
    }
    
    if (found_block) {
        *block_out = req_block;
        NET_INFRA_LOG(LOG_DEBUG,"allocated block offset=%u, level=%u", req_block->offset, req_block->level);    
        return 0;
    }

    //update level parameter to the non empty level (search inside the linked list for non empty level starting from the target level and up to the larger blocks levels)
    rc = find_non_empty_level(b_alloc, level, &level);

    if ( rc!= 0) {
        return rc;
    }

    rc = split_block_down(b_alloc, level, original_block_level, &req_block);

    if ( rc!= 0) {
        return rc;
    }
    
    req_block->is_allocated = true;
    NET_INFRA_LOG(LOG_DEBUG,"allocated block offset=%u, level=%u", req_block->offset, req_block->level);
    *block_out = req_block;

    return 0;
}

int buddy_alloc_free(buddy_alloc_t *b_alloc, struct buddy_alloc_block *block) {

    int rc;
    uint32_t block_size, buddy_offset, block_offset; 
    int current_block_level;
    bool buddy_exist = false;

    if (unlikely(b_alloc == NULL || block == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    if (unlikely(block->level >= b_alloc->num_of_levels)) {
        NET_INFRA_LOG(LOG_ERROR, "block->level > max level (offset=%u)", block->offset);
        return -EINVAL;
    }
    
    struct buddy_alloc_block *current_block = block;
    current_block_level = current_block->level;
    block_offset = current_block->offset;
    block_size = get_block_size(current_block_level, b_alloc->min_block_size, b_alloc->num_of_levels);
    current_block->is_active = false;

    while (current_block_level > 0)
    {
        rc = remove_buddy_block(b_alloc, &block_offset, &current_block_level, &block_size, &buddy_offset, &buddy_exist);
        
        if (unlikely(rc != 0)) {
            return rc;            
        }

        if (!buddy_exist) {
            break;
        }

        block_size <<= 1;
        block_offset = MIN(block_offset, buddy_offset);
        current_block_level--;
    }

    rc = create_new_block(b_alloc, current_block_level, block_offset, block_size, &current_block);
    if (unlikely(rc != 0)) {
        return rc;
    }

    rc = linked_list_add_block(b_alloc, current_block);
    if (unlikely(rc != 0)) {
        return rc;
    }

    NET_INFRA_LOG(LOG_DEBUG,"freed block offset=%u, ended at level=%d", block->offset, current_block_level);
    return 0;
}

int buddy_alloc_get_num_of_levels(const buddy_alloc_t *b_alloc, uint32_t *num_of_levels) {

    if (unlikely(b_alloc == NULL || num_of_levels == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    *num_of_levels = b_alloc->num_of_levels;

    return 0;
}

int buddy_alloc_count_free_blocks(const buddy_alloc_t *b_alloc, uint32_t level, uint32_t *free_blocks_num) {

    int count = 0;

    if (unlikely(b_alloc == NULL || free_blocks_num == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }
    
    struct buddy_alloc_block *cur = b_alloc->free_list[level].head;
    while (cur) {
        ++count;
        cur = cur->next;
    }

    *free_blocks_num = count;
    return 0;
}

int buddy_alloc_get_block_level(const struct buddy_alloc_block *blk, uint32_t *blk_level) {

    if (blk == NULL || blk_level == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    *blk_level = blk->level;
    return 0;
}

int buddy_alloc_get_block_offset(const struct buddy_alloc_block *blk, uint32_t *blk_offset) {

    if (blk == NULL || blk_offset == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    *blk_offset = blk->offset;
    return 0;
}
