#include <stdio.h>
#include <stdint.h>

typedef struct  {
    uint8_t *data;
    uint32_t data_len;
} store_mngr_data_t;

enum store_type {
    TYPE1,
    TYPE2,
    TYPE3
};

typedef struct  {
    uint32_t total_size;
    uint32_t minimum_block_size;
    enum store_type type;
} store_mngr_cfg_t;


typedef struct store_mngr store_mngr_t;
typedef struct store_mngr_block store_mngr_block_t;


int store_mngr_create(store_mngr_t **s_mngr, store_mngr_cfg_t *s_mngr_cfg);
int store_mngr_destroy(store_mngr_t *s_mngr);
int store_mngr_block_alloc(store_mngr_t *s_mngr, uint32_t file_size, store_mngr_block_t **mem_block_out);
int store_mngr_block_free(store_mngr_t *s_mngr, store_mngr_block_t *b_handler);
int store_mngr_block_write(store_mngr_t *s_mngr, store_mngr_block_t *b_handler, uint32_t file_offset, store_mngr_data_t data);
int store_mngr_block_read(store_mngr_t *s_mngr, store_mngr_block_t *b_handler, uint32_t file_offset, uint32_t read_size, store_mngr_data_t *data_out);