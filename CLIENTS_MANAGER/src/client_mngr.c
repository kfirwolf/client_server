#include <id_pool.h>
#include <common_defs.h>
#include <infra_log.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include "client_mngr.h"

//#define MAX_CLIENTS 256
//#define MAX_FILES_PER_CLIENT 1024

typedef struct  {
    uint32_t file_size;
    void *ctx;
} file_data;

typedef struct  {
    id_pool_t *files_id_pool;
    file_data *files_data;
    bool in_use;
} client_data;

struct client_mngr_t {
    uint32_t max_clients;
    uint32_t max_files;
    id_pool_t *clients_id_pool;
    client_data clients_data[];
};

static inline file_data *get_file_data(client_mngr_t *mgr, uint32_t client_id, uint32_t file_id) {
    return &mgr->clients_data[client_id].files_data[file_id];
}


int client_mngr_create(client_mngr_cfg_t *client_manager_cfg, client_mngr_t **client_manager) {
    int rc = 0;
    id_pool_cfg_t client_pool_cfg;
    id_pool_cfg_t file_pool_cfg;    
    client_mngr_t *manager = NULL;

    if (unlikely(client_manager == NULL || client_manager_cfg == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");
        return -EINVAL;
    }

    //calc allocation sizes
    const uint32_t base_size = sizeof(client_mngr_t);
    const uint32_t clients_data_size = client_manager_cfg->clients_capacity * sizeof(client_data);
    const uint32_t files_data_size = client_manager_cfg->files_capacity * sizeof(file_data);
    const size_t total_size = base_size + clients_data_size + files_data_size;

    manager = calloc(1, total_size);

    if (unlikely(manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "Memory allocation failed");
        return -ENOMEM;
    }

    manager->max_clients = client_manager_cfg->clients_capacity;
    manager->max_files = client_manager_cfg->files_capacity;
    manager->clients_id_pool = NULL;

    client_pool_cfg.capacity = client_manager_cfg->clients_capacity;
    client_pool_cfg.id_start_offset = 0;
    file_pool_cfg.capacity = client_manager_cfg->files_capacity;
    file_pool_cfg.id_start_offset = 0;

    // Initialize client ID pool
    rc = id_pool_create(&(manager->clients_id_pool), &client_pool_cfg);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while creating clients id pool.");
        free(manager);
        manager = NULL;
        return rc;
    }

    // Set files_data pointers to pre-allocated region
    file_data *file_base = (file_data *)((char *)manager + base_size + clients_data_size);
    for (uint32_t i = 0; i < manager->max_clients; i++) {
        manager->clients_data[i].files_data  = file_base + (i * manager->max_files);

        // Initialize file ID pools upfront
        rc = id_pool_create(&(manager->clients_data[i].files_id_pool), &file_pool_cfg);

        if (rc != 0) {
            for (uint32_t j = 0; j < i; j++) {
                id_pool_destroy(manager->clients_data[j].files_id_pool);    
            }

            NET_INFRA_LOG(LOG_ERROR, "Error while creating files id pool.");
            id_pool_destroy(manager->clients_id_pool);
            free(manager);

            return rc;
        }
    }

    *client_manager = manager;
    NET_INFRA_LOG(LOG_INFO, "Client manager successfully created.");
    return 0;
}

int client_mngr_destroy(client_mngr_t *client_manager) {
    //before this function, the server should make sure all files on each client are deleted.
    int rc = 0;
    if (unlikely(client_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }
    
    for (size_t i = 0; i < client_manager->max_clients; i++) {
        rc = id_pool_destroy(client_manager->clients_data[i].files_id_pool);
        if (unlikely(rc != 0)) {
            NET_INFRA_LOG(LOG_ERROR, "Error while destroying file ID pools for client: %u.", i);
            return rc;
        }        

    }
    
    rc = id_pool_destroy(client_manager->clients_id_pool);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Error while destroying clients ID pool.");
        return rc;
    }
    
    free(client_manager);
    return 0;
}

int client_mngr_allocate(client_mngr_t *client_manager, uint32_t *client_id) {
    uint32_t c_id = 0;
    int rc = 0;

    if (unlikely(client_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }
    
    //should the id_pool_allocate function be blocking function, while we allocate a client no other client can allocate at the same time ?
    rc = id_pool_allocate(client_manager->clients_id_pool, &c_id);
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Client manager failed to allocate new client_id");        
        return rc;
    }

    client_manager->clients_data[c_id].in_use = true;

    *client_id = c_id;
    NET_INFRA_LOG(LOG_INFO, "Client manager successfully allocated new client_id: %u.", *client_id);
    return 0;
}

int client_mngr_free(client_mngr_t *client_manager, uint32_t client_id) {

    int rc = 0;
    uint32_t id_count = 0;

    if (unlikely(client_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }

    if (unlikely(client_id >= client_manager->max_clients)) {
            NET_INFRA_LOG(LOG_ERROR, "Illegal client_id value");        
        return -EINVAL;
    }

    if (unlikely(id_pool_get_used_id_count(client_manager->clients_data[client_id].files_id_pool, &id_count) != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to get used id count");
        return -EACCES;
    }

    if (unlikely(id_count != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Cant free client with existing files");
        //checking that no files exist in this client before freeing the client.
        return -EACCES;
    }    

    client_manager->clients_data[client_id].in_use = false;

    //free this specific client from the clients id data pool
    rc = id_pool_free(client_manager->clients_id_pool, client_id);

    if (rc != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Client manager free failed client_id: %u.", client_id);         
        return rc;
    }

    NET_INFRA_LOG(LOG_INFO, "Client manager successfully free client_id: %u.", client_id);    
    return 0;
}

int client_mngr_file_allocate(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_size, uint32_t *file_id) {
    uint32_t f_id = 0;
    int rc = 0;

    if (unlikely(client_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }

    if (unlikely(client_id >= client_manager->max_clients)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal client_id value");        
        return -EINVAL;
    }

    client_data *client = &(client_manager->clients_data[client_id]);

    rc = id_pool_allocate(client->files_id_pool, &f_id);

    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Client manager failed to allocated new file_id: %u in client id: %u", f_id, client_id);       
        return rc;
    }

    client->files_data[f_id].file_size = file_size;
    client->files_data[f_id].ctx = NULL;
    *file_id = f_id;
    NET_INFRA_LOG(LOG_INFO, "Client manager successfully allocated new file_id: %u.", *file_id);
    return 0;
}

int client_mngr_file_free(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_id) {

    int rc = 0;
    file_data *f_data = NULL;

    if (unlikely(client_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }

    if (unlikely(client_id >= client_manager->max_clients)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal client_id value");
        return -EINVAL;
    }

    if (unlikely(file_id >= client_manager->max_files)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal file_id value");
        return -EINVAL;
    }    

    f_data = get_file_data(client_manager, client_id, file_id);
    if (unlikely(f_data->ctx != NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "can't remove file object before actual file data is deleted");        
        return -EPERM;
    }

    f_data->file_size = 0;

    rc = id_pool_free(client_manager->clients_data[client_id].files_id_pool, file_id);
    
    if (unlikely(rc != 0)) {
        NET_INFRA_LOG(LOG_ERROR, "Client manager failed to free file_id: %u in client id: %u", file_id, client_id);
        return rc;
    }

    NET_INFRA_LOG(LOG_INFO, "Client manager successfully free file_id: %u.", file_id);
    return 0;
}

int client_mngr_file_set_ctx(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_id, void *ctx) {

    if (unlikely(client_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }

    if (unlikely(client_id >= client_manager->max_clients)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal client_id value");
        return -EINVAL;
    }

    if (unlikely(file_id >= client_manager->max_files)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal file_id value");
        return -EINVAL;
    }        

    get_file_data(client_manager, client_id, file_id)->ctx = ctx;
    NET_INFRA_LOG(LOG_INFO, "Client manager successfully set ctx to file_id: %u in client_id: %u.", file_id, client_id);
    return 0;
}

int client_mngr_file_get_ctx(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_id, void **ctx) {

    if (unlikely(client_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }

    if (unlikely(client_id >= client_manager->max_clients)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal client_id value");
        return -EINVAL;
    }

    if (unlikely(file_id >= client_manager->max_files)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal file_id value");
        return -EINVAL;
    } 

    if (unlikely(get_file_data(client_manager, client_id, file_id)->ctx == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL ctx");
        return -EINVAL;
    } 

    *ctx = get_file_data(client_manager, client_id, file_id)->ctx;

    return 0;
}

int client_mngr_file_get_size(client_mngr_t *client_manager, uint32_t client_id, uint32_t file_id, uint32_t *size) {

    if (unlikely(client_manager == NULL)) {
        NET_INFRA_LOG(LOG_ERROR, "NULL input args");        
        return -EINVAL;
    }

    if (unlikely(client_id >= client_manager->max_clients)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal client_id value");
        return -EINVAL;
    }

    if (unlikely(file_id >= client_manager->max_files)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal file_id value");
        return -EINVAL;
    }

    *size = get_file_data(client_manager, client_id, file_id)->file_size;
    return 0;
}