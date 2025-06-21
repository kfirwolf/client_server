
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <common_defs.h>
#include <infra_log.h>
#include <id_pool.h>
#include "buddy_allocator.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define MAX_LEVELS 64


typedef struct {
    id_pool_t *id_pool;
    void *elements_array;
    uint32_t num_of_elements;
    uint32_t size_of_element;
} mem_pool_t;


typedef struct {
    uint32_t element_size; 
    uint32_t num_elements;
} mem_pool_cfg_t;

int mem_pool_create(mem_pool_t **pool, mem_pool_cfg_t *mem_pool_cfg);
int mem_pool_allocate(mem_pool_t *pool, void **ptr);
int mem_pool_free(mem_pool_t *pool, void *ptr);
int mem_pool_destroy(mem_pool_t *pool);

struct block {
    struct block *next;
    uint32_t offset;
    uint32_t level : 6; //Instead of size will use level instead (we allocated only 6 bits for level and the rest will go to the new index we wanted to add will be what left)
    uint32_t block_id : 26;
};

struct buddy_free_list {
    struct block *head;
};

struct buddy_allocator {
    uint32_t min_block_size; 
    uint32_t num_of_levels;
    mem_pool_t *mem_pool;
    struct buddy_free_list free_list[];
};

static inline uint32_t get_buddy_offset(uint32_t block_offset, uint32_t block_size) {
    return block_offset ^ block_size;
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

static int get_block_level(const buddy_allocator_t *b_alloc, uint32_t block_size, uint32_t *level_out)
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

//////////////////////////////// INTERNAL MEM POOL API ////////////////////////////////////////////////////

int mem_pool_create(mem_pool_t **pool, mem_pool_cfg_t *mem_pool_cfg) {

    int rc = 0;
    id_pool_cfg_t cfg;
    *pool = calloc(1, sizeof(mem_pool_t));
    if (*pool == NULL) {
        return -ENOMEM;
    }

    (*pool)->elements_array = (struct block *)calloc(1, (mem_pool_cfg->num_elements * mem_pool_cfg->element_size));
    mem_pool_t *memory_pool = (mem_pool_t *)(*pool)->elements_array;

    if (unlikely(memory_pool == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "Node pool memory allocation failed");
        return -ENOMEM;
    }

    //setting id pool
    cfg.capacity = (*pool)->num_of_elements;
    cfg.id_start_offset = 0;
    rc = id_pool_create(&((*pool)->id_pool), &cfg);

    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while creating maximum nodes pool.");
        free((*pool)->elements_array);
        free(*pool);
        return rc;
    }

    return 0;
}

int mem_pool_destroy(mem_pool_t *pool) {

    int rc = 0;

    if (pool == NULL) {
        return -ENOMEM;
    }

    rc = id_pool_destroy(pool->id_pool);

    if (rc != 0) {
        return rc;
    }

    free(pool->elements_array);

    return 0;

}

int mem_pool_allocate(mem_pool_t *pool, void **ptr) {

    int rc = 0;
    uint32_t mem_pool_idx;

    if (pool == NULL) {
        return -ENOMEM;
    }

    rc = id_pool_allocate(pool->id_pool, &mem_pool_idx);

    if (rc != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to allocate new element");
        return rc;
    }

    *ptr = &pool->elements_array[mem_pool_idx];
    return 0;
}

int mem_pool_free(mem_pool_t *pool, void *ptr) {

}

//////////////////////////////// INTERNAL LINKED LIST API ////////////////////////////////////////////////////

static int linked_list_add_block_as_head(buddy_allocator_t *b_alloc, uint32_t level, uint32_t offset) {
    int rc = 0;
    uint32_t node_pool_idx = 0;    
    rc = id_pool_allocate(b_alloc->block_pool.max_nodes_id_pool, &node_pool_idx);

    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to allocate new node");
        return rc;
    }

    struct block *free_block = &b_alloc->block_pool.node_pool[node_pool_idx];
    free_block->next = b_alloc->free_list[level].head;
    free_block->offset = offset;
    free_block->level = level;
    free_block->block_id = node_pool_idx;
    b_alloc->free_list[level].head = free_block;

    return 0;
}

static int linked_list_extract_block_by_offset(buddy_allocator_t *b_alloc, uint32_t level, uint32_t offset, bool *block_exist) {

    int rc = 0;
    struct block *block_to_remove = NULL;
    struct block *current_block = b_alloc->free_list[level].head;
    if ((unlikely(current_block == NULL))) {
        NET_INFRA_LOG(LOG_ERROR, "Could not remove node, no list in level: %u", level);
        return -EINVAL;
    }

    if (unlikely(level >= b_alloc->num_of_levels)) {
        NET_INFRA_LOG(LOG_ERROR, "Invalid level: %u", level);
        return -EINVAL;
    }    


    if (current_block->offset == offset) {
        b_alloc->free_list[level].head = current_block->next;
        block_to_remove = current_block;
    }
    else {

        while (current_block->next != NULL && current_block->next->offset != offset) {
            current_block = current_block->next;
        }
    
        if (unlikely(current_block->next == NULL)) {
            NET_INFRA_LOG(LOG_INFO, "Could not remove node, cant find block with offset: %u in level: %u", offset, level);
            if (block_exist != NULL) {
                *block_exist = false;
            }
            return 0;
        }
    
        block_to_remove = current_block->next;
        current_block->next = block_to_remove->next;
    }

    rc = id_pool_free(b_alloc->block_pool.max_nodes_id_pool, block_to_remove->block_id);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to free node id");
        return rc;
    }

    if (block_exist != NULL) {
        *block_exist = true;
    }
    return 0;
}

static int linked_list_search_block(buddy_allocator_t *b_alloc, uint32_t level, uint32_t offset) {
    struct block *list_head = b_alloc->free_list[level].head;    
}

//////////////////////////////// INTERNAL BUDDY ALLOCATOR API ////////////////////////////////////////////////////

static int merge_with_buddy(buddy_allocator_t *b_alloc, int *level, uint32_t *block_offset, uint32_t *block_size, bool *found_buddy) {
    int rc = 0;
    *found_buddy = false;
    uint32_t buddy_offset = get_buddy_offset(*block_offset, *block_size);
    bool buddy_exist = false;
    
    if (!b_alloc->free_list[*level].head) {
        *found_buddy = false;
        return 0;
    }

    rc = linked_list_extract_block_by_offset(b_alloc, *level, buddy_offset, &buddy_exist);
    if (rc != 0) {
        return rc;
    }

    if (!buddy_exist) {
        *found_buddy = false;
        return 0;
    }

    *block_size <<= 1;
    *block_offset = MIN(*block_offset, buddy_offset);
    (*level)--; 

    *found_buddy = true;
    return 0;
}

static int try_allocate_at_level(buddy_allocator_t *b_alloc, uint32_t level, uint32_t size, block_t *block_out, bool *found_block) {
    int rc = 0;
    struct block* target_block = NULL;
    target_block = b_alloc->free_list[level].head;

    if (target_block == NULL) {
        *found_block = false;
        return 0;
    }

    block_out->offset = target_block->offset;
    block_out->size = size;
    rc = linked_list_extract_block_by_offset(b_alloc, level, target_block->offset, NULL);
    if (unlikely(rc != 0)) {
        return rc;            
    }

    *found_block = true;

    return 0;
}

static inline int find_non_empty_level(buddy_allocator_t *b_alloc, uint32_t start_level, uint32_t start_size, uint32_t *found_level, uint32_t *found_size) {
    uint32_t level = start_level;
    uint32_t split_size = start_size;

    while (level > 0 && b_alloc->free_list[level].head == NULL) {
        level--;
        split_size = split_size << 1; // 2 times larger size
    }

    if (b_alloc->free_list[level].head == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "No free space for allocating a block");
        return  -ENFILE;     
    }

    *found_level = level;
    *found_size = split_size;

    return 0;
}

static int split_block_down(buddy_allocator_t *b_alloc, uint32_t src_level, uint32_t target_level, uint32_t *block_offset, uint32_t *block_size) {
    int rc = 0;
    uint32_t level = src_level;
    block_t *current_block = b_alloc->free_list[src_level].head;
    uint32_t b_offset = current_block->offset;
    uint32_t split_size =  *block_size;
    uint32_t buddy_offset = 0;

    // remove the src block
    rc = linked_list_extract_block_by_offset(b_alloc, src_level, b_offset, NULL);
    if (unlikely(rc != 0)) {
        return rc;            
    }

    while (++level <= target_level) { 
        split_size = split_size >> 1; //2 times smaller size
        buddy_offset = get_buddy_offset(b_offset, split_size);
        rc = linked_list_add_block_as_head(b_alloc, level, buddy_offset);
        if (rc != 0) {
            return rc;
        }
    }
    
    *block_offset = b_offset;
    *block_size = split_size;

    return 0;
}

//////////////////////////////////////////// EXTERNAL BUDDY ALLOCATOR API ///////////////////////////////////////////////////////

int buddy_alloc_create(buddy_alloc_cfg_t *b_cfg, buddy_allocator_t **b_alloc) {
    int rc = 0;
    buddy_allocator_t *b_allocator = NULL;
    id_pool_cfg_t client_pool_cfg;
    uint32_t node_pool_idx = 0;
    mem_pool_cfg_t mem_pool_cfg;


    if (unlikely(b_alloc == NULL || b_cfg == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }    

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
    const uint32_t base_size = sizeof(buddy_allocator_t);
    const uint32_t free_list_size = num_of_levels * sizeof(struct buddy_free_list);
    const uint32_t max_nodes_size = max_nodes * sizeof(struct block);

    const size_t total_size = base_size + free_list_size;
    b_allocator = calloc(1, total_size);

    if (unlikely(b_allocator == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "b_allocator memory allocation failed");
        return -ENOMEM;
    }

    mem_pool_cfg.num_elements = max_nodes;
    mem_pool_cfg.element_size = sizeof(struct block);

    rc = mem_pool_create(&b_allocator->mem_pool, &mem_pool_cfg);
    if (unlikely(rc != 0)) {
        free(b_allocator);
        return rc;
    }
    
    //setting buddy allocator
    b_allocator->min_block_size = minb;
    b_allocator->num_of_levels = num_of_levels;

    rc = id_pool_allocate(b_allocator->block_pool.max_nodes_id_pool, &node_pool_idx);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to allocate new node");
        id_pool_destroy(b_allocator->block_pool.max_nodes_id_pool);
        free(b_allocator->block_pool.node_pool);
        free(b_allocator);
        return rc;
    }    

    //setting first (largest block) level linked list
    struct block *free_block = &b_allocator->block_pool.node_pool[node_pool_idx];
    free_block->next = NULL;
    free_block->offset = 0;
    free_block->level = 0;
    free_block->block_id = node_pool_idx;

    // Level 0 == largest blocks (entire pool)
    // Level N-1 == smallest blocks (min_block_size)
    b_allocator->free_list[free_block->level].head = free_block;
    *b_alloc = b_allocator;
    NET_INFRA_LOG(LOG_INFO, "Buddy allocator successfully created");    
    return 0;
}

int buddy_alloc_destroy(buddy_allocator_t *b_alloc) {
    if (unlikely(b_alloc == NULL)) {
        return -EINVAL;
    }

    //Tear down the ID‐pool you used for node indices
    id_pool_destroy(b_alloc->block_pool.max_nodes_id_pool);

    //Free node pool
    free(b_alloc->block_pool.node_pool);

    //Free the block that holds your buddy_allocator_t, free_list[], 
    free(b_alloc);
    NET_INFRA_LOG(LOG_INFO, "Buddy allocator destroyed");
    return 0;
}

int buddy_alloc_allocate(buddy_allocator_t *b_alloc, uint32_t block_size, block_t *block_out) {
    int rc;
    uint32_t original_block_level, level, block_offset, buddy_offset, split_size;
    split_size = block_size;

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
    rc = try_allocate_at_level(b_alloc, level, split_size, block_out, &found_block);
    if ( rc!= 0) {
        return rc;
    }
    
    if (found_block) {
        NET_INFRA_LOG(LOG_DEBUG,"allocated block offset=%u, size=%u, level=%d", block_out->offset, block_out->size, original_block_level);
        return 0;
    }

    rc = find_non_empty_level(b_alloc, level, split_size, &level, &split_size);

    if ( rc!= 0) {
        return rc;
    }

    rc = split_block_down (b_alloc, level, original_block_level, &block_offset, &split_size);

    if ( rc!= 0) {
        return rc;
    }
    
    block_out->offset =  block_offset;
    block_out->size = split_size;
    NET_INFRA_LOG(LOG_DEBUG,"allocated block offset=%u size=%u, level=%d", block_out->offset, block_out->size, original_block_level);
    return 0;
    /*
        1. calc round up power of 2 from block_size.b_alloc
        2. calc free list index from power of 2 block size (target_level)
        3. search from target level upward toward larger blocks (to smaller indeces), untill a free block is found (upward = larger size blocks)
        4. If you found a block at a higher level (larger block), split it recursively.
            4.a. found block , remove it from the free list ,  
            4.b. split block in half , take first half and add second half to the free_list[level]
            4.c. continue splitting this block untill you get to the target_level.
        5. return the target level block (first buddy)
        * i need to create API for adding , removing elements from linked list
    */
}

int buddy_alloc_free(buddy_allocator_t *b_alloc, const block_t *block) {

    int rc;
    uint32_t block_size, buddy_offset, block_offset; 
    int current_block_level = 0;
    bool buddy_block_exist = false;

    if (unlikely(b_alloc == NULL || block == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    if (unlikely(block->size == 0)) {
        NET_INFRA_LOG(LOG_ERROR, "block size == 0 (offset=%u)", block->offset);
        return -EINVAL;
    }

    block_size = next_pow2(block->size);
    rc = get_block_level(b_alloc, block_size, &current_block_level);

    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "illegal block level value: %d", current_block_level);        
        return rc;            
    }

    block_offset = block->offset;

    // Calculate buddy offset: buddy = offset ^ size
    // e.g., 2 ^ 4 = 6 → buddy of 2 is 6; 6 ^ 4 = 2 → buddy of 6 is 2
    buddy_offset = get_buddy_offset(block_offset, block_size);
    bool buddy_exist = false;

    while (current_block_level > 0)
    {
        rc =  merge_with_buddy(b_alloc, &current_block_level, &block_offset, &block_size, &buddy_exist);
        
        if (unlikely(rc != 0)) {
            return rc;            
        }

        if (!buddy_exist) {
            break;
        }
    }

    rc = linked_list_add_block_as_head(b_alloc, current_block_level, block_offset);
    if (unlikely(rc != 0)) {
        return rc;            
    }

    NET_INFRA_LOG(LOG_DEBUG,"freed block offset=%u size=%u ended at level=%d", block->offset, block->size, current_block_level);
    return 0;
}
