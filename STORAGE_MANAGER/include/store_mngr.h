#ifdef __cplusplus
extern "C" {
#endif

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
    uint32_t store_size;
    enum store_type type;
    uint32_t min_block_size;    
    bool clear_when_free_block; // add impl to this flag to zero data when freeing a block according to this flag
} store_mngr_cfg_t;

typedef struct store_mngr store_mngr_t;
typedef struct store_mngr_block store_mngr_block_t;
typedef struct store_mngr_block store_mngr_block_t;


int store_mngr_create(store_mngr_t **s_mngr, store_mngr_cfg_t* s_mngr_cfg);
int store_mngr_destroy(store_mngr_t *s_mngr);
int store_mngr_block_alloc(store_mngr_t *s_mngr, uint32_t block_size, store_mngr_block_t **b_handler);
int store_mngr_block_free(store_mngr_t *s_mngr, store_mngr_block_t *b_handler);
int store_mngr_block_write(store_mngr_t *s_mngr, store_mngr_block_t *b_handler, uint32_t byte_offset, store_mngr_data_t data);
int store_mngr_block_read(store_mngr_t *s_mngr, store_mngr_block_t *b_handler, uint32_t byte_offset, uint8_t *buffer, uint32_t buffer_len);
int store_mngr_get_type(store_mngr_t *s_mngr, enum store_type* s_type);

#ifdef __cplusplus
}
#endif