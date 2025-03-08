#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "net_client.h"
#include "ram_file_ctrl.h"
#include "net_infra.h"
#include "file_transfer_protocol.h"


#define MAX_FILE_SIZE (100 * 1024 + 121)
#define MAX_FILE_BUFFER_SIZE (1024)            // 1 KB
#define FILE_PATH "/tmp/send_ram_file_ctrl.bin"
#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     54322
#define NUM_OF_CHUNKS_IN_WRITE     1
#define TOTAL_SIZE sizeof(data_chunk_protocol) + MAX_FILE_BUFFER_SIZE

static size_t file_size = 0;

static int creating_data_file(const char *path, size_t f_size) {
    file_size = f_size;
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "Failed to open file");
        return -1;
    }

    uint8_t buffer[MAX_FILE_BUFFER_SIZE]; // 1KB buffer
    for (size_t i = 0; i < f_size / sizeof(buffer); i++) {
        /* Fill buffer with random 8-bit values */
        for (size_t j = 0; j < sizeof(buffer); j++) {
            buffer[j] = rand() % 256; // Random 8-bit value
        }

        if (fwrite(buffer, sizeof(buffer), NUM_OF_CHUNKS_IN_WRITE, file) != NUM_OF_CHUNKS_IN_WRITE) {
            NET_INFRA_LOG(LOG_ERROR, "Failed writing to file");
            fclose(file);
            return -1; 
        }
    }

    size_t remaining  = (f_size) % sizeof(buffer);

    if (remaining  > 0) {
        for (size_t j = 0; j < remaining ; j++) {
            buffer[j] = rand() % 256; // Random 8-bit value
        }

        if (fwrite(buffer, 1, remaining , file) != remaining) {
            NET_INFRA_LOG(LOG_ERROR, "Failed writing remaining bytes to file");
            fclose(file);
            return -1; 
        }        
    }

    fclose(file);
    NET_INFRA_LOG(LOG_INFO, "Binary file '%s' created successfully with ascending numeric numbers.\n", path);

    return 0;
}

int main() {
    net_infra_set_log_level(LOG_INFO);
    NET_INFRA_LOG(LOG_INFO, "Starting main sender");

    if(creating_data_file(FILE_PATH, MAX_FILE_SIZE) != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed creating bin file");
        return -1;
    }

    ram_file_t *file_mngr;
    ram_file_cfg_t file_config;
    file_config.max_file_size = MAX_FILE_SIZE;
    file_config.delete_files = false;
    file_config.file_path = FILE_PATH;

    if (ram_file_create(&file_config, &file_mngr)) {
        NET_INFRA_LOG(LOG_ERROR, "Failed creating ram_file");
        return -1;
    }

    NET_INFRA_LOG(LOG_INFO, "Succesfully created ram_file");

    net_client_t *net_client;
    net_client_cfg_t net_client_cfg;
    net_client_cfg.server_ip = SERVER_IP;
    net_client_cfg.server_port = SERVER_PORT;

    if (net_client_create(&net_client_cfg, &net_client) != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed creating client");
        return -1;        
    }

    NET_INFRA_LOG(LOG_INFO, "Succesfully created client");

    uint8_t data_chunk_buffer[TOTAL_SIZE] = {0};
    data_chunk_protocol *data_chunk = (data_chunk_protocol *)data_chunk_buffer;
    net_client_data_t send_data;
    net_client_iov_t iov;

    /* Setup I/O vector: total length includes header + payload */
    iov.data = (uint8_t *)data_chunk;

    send_data.iov_count = 1;
    send_data.iov = &iov;
    
    int file_read_err = 0;
    size_t offset = 0;

    while (offset < file_size) {
        size_t data_chunk_size = (file_size - offset < MAX_FILE_BUFFER_SIZE) ? (file_size - offset) : MAX_FILE_BUFFER_SIZE;
        file_read_err = ram_file_read(file_mngr, offset, data_chunk->data, data_chunk_size);
        if (file_read_err != 0) {
            break;
        }
        NET_INFRA_LOG(LOG_INFO, "data_chunk_size: %zu", data_chunk_size);
        data_chunk->flags = 0;
        data_chunk->offset = offset;
        iov.data_len = data_chunk_size + sizeof(data_chunk_protocol);

        if (net_client_send(net_client, &send_data) != 0) {
            NET_INFRA_LOG(LOG_ERROR, "Failed sending data");
            return -1;        
        }

        offset+= data_chunk_size;
        //usleep(1000 * 1000);
    }

    if (file_read_err != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Error %d: %s\n", file_read_err, strerror(file_read_err));
        return -1;
    }

    return 0;
}