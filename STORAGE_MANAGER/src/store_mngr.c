#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "store_mngr.h"
#include "buddy_allocator.h"
#include "ram_file_ctrl.h"
#include <infra_log.h>

struct store_mngr {
    buddy_alloc_t *b_alloc;
    ram_file_t *r_file_ctrl;
    enum store_type type;
};

struct store_mngr_block {
    buddy_alloc_block_t *b_alloc_block;
};

int store_mngr_create(store_mngr_t **s_mngr, store_mngr_cfg_t* s_mngr_cfg) {
    
    if (s_mngr == NULL || s_mngr_cfg == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "storage manager invalid values");
        return -EINVAL;
    }

    int rc = 0;

    store_mngr_t *_s_mngr = (store_mngr_t *)calloc(1, sizeof(store_mngr_t));

    if (_s_mngr == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "storage manager allocation failed");
        return -ENOMEM;
    }
    _s_mngr->type = s_mngr_cfg->type;

    buddy_alloc_cfg_t b_cfg;
    b_cfg.min_block_size = s_mngr_cfg->minimum_block_size;
    b_cfg.total_mem_pool_size = s_mngr_cfg->total_size;

    rc = buddy_alloc_create(&b_cfg, &(_s_mngr->b_alloc));

    if (rc != 0) {
        goto fail;
    }

    ram_file_cfg_t r_file_cfg;
    r_file_cfg.delete_files = true;
    r_file_cfg.max_file_size = s_mngr_cfg->total_size;
    r_file_cfg.file_path = "./storage.bin"; //relative path from current folder 


    rc = ram_file_create(&r_file_cfg, &(_s_mngr->r_file_ctrl));
    if ( rc != 0) {
        goto fail;
    }

    *s_mngr =  _s_mngr;
    return 0;

    fail:
        if (_s_mngr) {
            if (_s_mngr->b_alloc) {
                buddy_alloc_destroy(_s_mngr->b_alloc);
            }
            if (_s_mngr->r_file_ctrl) {
                ram_file_destroy(_s_mngr->r_file_ctrl);
            }
            free(_s_mngr);
        }
        return rc;
}

int store_mngr_destroy(store_mngr_t *s_mngr) {
    if (s_mngr == NULL) {
        NET_INFRA_LOG(LOG_WARNING, "storage manager invalid values");
        return -EINVAL;
    }

    if (s_mngr->b_alloc) {
        buddy_alloc_destroy(s_mngr->b_alloc);
    }
    if (s_mngr->r_file_ctrl) {
        ram_file_destroy(s_mngr->r_file_ctrl);
    }
    free(s_mngr);
    return 0;
}

int store_mngr_block_alloc(store_mngr_t *s_mngr, uint32_t file_size, store_mngr_block_t **st_blk_handler) {

    if (s_mngr == NULL || st_blk_handler == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "storage manager alloc invalid values");
        return -EINVAL;
    }    

    int rc = 0;

    store_mngr_block_t *_st_blk_handler = calloc(1, sizeof(store_mngr_block_t));
    if (_st_blk_handler == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "storage manager aloc allocation failed");
        return -ENOMEM;
    }
    
    rc = buddy_alloc_allocate(s_mngr->b_alloc, file_size, &(_st_blk_handler->b_alloc_block));
    if (rc != 0) {
        goto fail;
    }

    *st_blk_handler = _st_blk_handler;
    return 0;

    fail: 
    free(_st_blk_handler);
    return rc;
}

int store_mngr_block_free(store_mngr_t *s_mngr, store_mngr_block_t *st_blk_handler) {

    if (s_mngr == NULL || st_blk_handler == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "storage manager alloc invalid values");
        return -EINVAL;
    }    

    int rc = 0;
    rc = buddy_alloc_free(s_mngr->b_alloc, st_blk_handler->b_alloc_block);

    free(st_blk_handler);
    return rc;
}

int store_mngr_block_write(store_mngr_t *s_mngr, store_mngr_block_t *st_blk_handler, uint32_t file_bytes_offset, store_mngr_data_t data) {

    if (s_mngr == NULL || st_blk_handler == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "storage manager alloc invalid values");
        return -EINVAL;
    }
    
    int rc = 0;
    uint32_t block_offset;
    rc = buddy_alloc_get_block_offset(st_blk_handler->b_alloc_block, &block_offset);
    if (rc != 0) {
        return rc;
    }


    uint32_t total_offset = block_offset + file_bytes_offset;
    rc = ram_file_write(s_mngr->r_file_ctrl, (size_t)total_offset,  data.data, (size_t)data.data_len);
    if (rc != 0) {
        return rc;
    }
    
    return 0;

}

int store_mngr_block_read(store_mngr_t *s_mngr, store_mngr_block_t *st_blk_handler, uint32_t file_bytes_offset, uint32_t read_size, store_mngr_data_t *data_out) {

    if (s_mngr == NULL || st_blk_handler == NULL || data_out == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "storage manager alloc invalid values");
        return -EINVAL;
    }
    
    int rc = 0;
    uint32_t block_offset;
    rc = buddy_alloc_get_block_offset(st_blk_handler->b_alloc_block, &block_offset);
    if (rc != 0) {
        return rc;
    }
    
    uint32_t total_offset = block_offset + file_bytes_offset;

    rc = ram_file_read(s_mngr->r_file_ctrl, total_offset, data_out->data, read_size);
    if (rc != 0) {
        return rc;
    }
    data_out->data_len = read_size;
    return 0;

}