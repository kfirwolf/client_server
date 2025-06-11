#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <common_defs.h>
#include <infra_log.h>
#include <ram_file_ctrl.h>
#include "store_mngr.h"

#define FILE_PATH "/home/kfirwolf/projects/total_mem_block"

struct store_mngr {
    uint32_t store_size;
    enum store_type type;
    ram_file_t *ram_file_mngr;
};

int store_mngr_create(store_mngr_t **s_mngr, store_mngr_cfg_t* s_mngr_cfg) {

    int rc = 0;
    ram_file_cfg_t r_file_cfg;
    store_mngr_t *store_manager = NULL;

    if (unlikely(s_mngr == NULL || s_mngr_cfg == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    store_manager = calloc(1, sizeof(store_mngr_t));

    if (unlikely(store_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "Memory allocation failed");
        return -ENOMEM;
    }

    store_manager->store_size = s_mngr_cfg->store_size;
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

    *s_mngr = store_manager;
    return 0;

}

int store_mngr_destroy(store_mngr_t *s_mngr) {
    int rc = 0;
    if (unlikely(s_mngr == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }
    
    // destroy the free blocks manager here

    rc = ram_file_destroy(s_mngr->ram_file_mngr);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while destroying ram file manager.");
        return rc;
    }
    s_mngr->ram_file_mngr = NULL;

    free(s_mngr);
    return 0;
}

store_mngr_block_t *store_mngr_block_alloc(store_mngr_t *s_mngr, uint32_t file_size) {

}

int store_mngr_block_free(store_mngr_block_t *b_handler) {

}

int store_mngr_block_write(store_mngr_block_t *b_handler, uint32_t file_offset, store_mngr_data_t data) {

}

int store_mngr_block_read(store_mngr_block_t *b_handler, uint32_t file_offset, uint8_t *buffer, uint32_t buffer_len, uint32_t  *out_len) {

}