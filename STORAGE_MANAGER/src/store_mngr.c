#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <common_defs.h>
#include <infra_log.h>
#include <ram_file_ctrl.h>
#include <buddy_allocator.h>
#include <id_pool.h>
#include "store_mngr.h"

#define FILE_PATH "/home/kfirwolf/projects/total_mem_block"
#define MAX_MEM_POOL_SIZE_IN_BYTES 1048576
#define MIN_MEM_POOL_BLOCK_SIZE_IN_BYTES 4096

struct store_mngr_block; 

struct store_mngr {
    uint32_t store_size;
    uint32_t min_block_size;    
    enum store_type type;
    ram_file_t *ram_file_mngr;
    buddy_allocator_t *buddy_alloc;
};

static inline int validate_offset(store_mngr_block_t *b_handler, uint32_t offset, uint32_t data_len) {

    if (offset + data_len > b_handler->block.size) {
        NET_INFRA_LOG(LOG_ERROR, "write out of bounds");
        return -EINVAL;
    }
}

int store_mngr_create(store_mngr_t **s_mngr, store_mngr_cfg_t* s_mngr_cfg) {

    int rc = 0;
    ram_file_cfg_t r_file_cfg;
    store_mngr_t *store_manager = NULL;
    buddy_alloc_cfg_t buddy_alloc_cfg;

    if (unlikely(s_mngr == NULL || s_mngr_cfg == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    store_manager = calloc(1, sizeof(store_mngr_t));

    if (unlikely(store_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "Memory allocation failed");
        return -ENOMEM;
    }

    if (s_mngr_cfg->store_size == 0) {
        store_manager->store_size = MAX_MEM_POOL_SIZE_IN_BYTES;
    }
    else {
        store_manager->store_size = s_mngr_cfg->store_size;
    }

    if (s_mngr_cfg->min_block_size == 0) {
        store_manager->min_block_size = MIN_MEM_POOL_BLOCK_SIZE_IN_BYTES;
    }
    else {
        store_manager->min_block_size = s_mngr_cfg->min_block_size;
    }

    store_manager->type = s_mngr_cfg->type;

    r_file_cfg.delete_files = false;
    r_file_cfg.file_path = FILE_PATH;
    r_file_cfg.max_file_size = store_manager->store_size;
    rc = ram_file_create(&r_file_cfg, &store_manager->ram_file_mngr);

    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while creating file using: ram file manager.");
        free(store_manager);
        return rc;
    }

    // create the free blocks manager here
    buddy_alloc_cfg.min_block_size = store_manager->min_block_size;
    buddy_alloc_cfg.total_mem_pool_size = store_manager->store_size;
    rc = buddy_alloc_create(&buddy_alloc_cfg, &store_manager->buddy_alloc);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while creating allocating system using: buddy allocator.");        
        ram_file_destroy(store_manager->ram_file_mngr);
        free(store_manager);
        return rc;
    }

    *s_mngr = store_manager;
    return 0;

}

int store_mngr_destroy(store_mngr_t *s_mngr) {
    int rc = 0;
    if (unlikely(s_mngr == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }

    rc = ram_file_destroy(s_mngr->ram_file_mngr);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while destroying ram file manager.");
        return rc;
    }

    rc = buddy_alloc_destroy(s_mngr->buddy_alloc);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while destroying buddy allocator.");
        return rc;
    }    

    s_mngr->ram_file_mngr = NULL;

    free(s_mngr);
    return 0;
}

int store_mngr_block_alloc(store_mngr_t *s_mngr, uint32_t block_size, store_mngr_block_t **b_handler) {

    int rc = 0;
    block_t *buddy_block;

    if (unlikely(s_mngr == NULL || b_handler == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    rc = buddy_alloc_allocate(s_mngr->buddy_alloc, block_size, buddy_block);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "store_mngr_block_alloc failed");        
        return rc;
    }

    //should i write somthing in the allocated mem offset using the ram file ctrl ? (something to mark this memory section as allocated)

    
    return 0;
}

int store_mngr_block_free(store_mngr_t *s_mngr, store_mngr_block_t *b_handler) {

    int rc = 0;

    if (unlikely(s_mngr == NULL || b_handler == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    rc = buddy_alloc_free(s_mngr->buddy_alloc, &b_handler->block);

    if (unlikely(rc != 0 )) {
        NET_INFRA_LOG(LOG_ERROR, "store_mngr_block_free failed");
        return rc;
    }

    //should we zero all block data when freeing ?
    return 0;
}

int store_mngr_block_write(store_mngr_t *s_mngr, store_mngr_block_t *b_handler, uint32_t byte_offset, store_mngr_data_t data) {

    int rc = 0;
    uint32_t offset = 0;

    if (unlikely(s_mngr == NULL || b_handler == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    offset = b_handler->block.offset + byte_offset;

    rc = validate_offset(b_handler, offset, data.data_len);
    if (unlikely(rc != 0 )) {
        return rc;
    }    

    rc = ram_file_write(s_mngr->ram_file_mngr, offset, data.data, (size_t)data.data_len);
    if (unlikely(rc != 0 )) {
        NET_INFRA_LOG(LOG_ERROR, "ram_file_write failed");
        return rc;
    }

    return 0;
}

int store_mngr_block_read(store_mngr_t *s_mngr, store_mngr_block_t *b_handler, uint32_t byte_offset, uint8_t *buffer, uint32_t buffer_len) {

    int rc = 0;
    uint32_t offset = 0;

    if (unlikely(s_mngr == NULL || b_handler == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    offset = b_handler->block.offset + byte_offset;

    rc = validate_offset(b_handler, offset, buffer_len);
    if (unlikely(rc != 0 )) {
        return rc;
    }    

    rc = ram_file_read(s_mngr->ram_file_mngr, offset, buffer, (size_t)buffer_len);

    if (unlikely(rc != 0 )) {
        NET_INFRA_LOG(LOG_ERROR, "ram_file_read failed");
        return rc;
    }

    return 0;
}

int store_mngr_get_type(store_mngr_t *s_mngr, enum store_type* s_type) {
    int rc = 0;

    if (unlikely(s_mngr == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    *s_type = s_mngr->type;
    return 0;
}