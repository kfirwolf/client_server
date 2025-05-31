#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct client_mngr_t client_mngr_t;

typedef struct {
    uint32_t clients_capacity;
    uint32_t files_capacity;
} client_mngr_cfg_t;


int client_mngr_create(client_mngr_cfg_t *client_mananger_cfg, client_mngr_t **client_manager);
int client_mngr_destroy(client_mngr_t *client_manager);
int client_mngr_allocate(client_mngr_t *client_manager, uint32_t *client_id);
int client_mngr_free(client_mngr_t *client_manager, uint32_t client_id);
int client_mngr_file_allocate(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_size, uint32_t *file_id);
int client_mngr_file_free(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_id);
int client_mngr_file_set_ctx(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_id, void *ctx);
int client_mngr_file_get_ctx(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_id, void **ctx);

//return file size
int client_mngr_file_get_size(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_id, uint32_t *size);

#ifdef __cplusplus
}
#endif