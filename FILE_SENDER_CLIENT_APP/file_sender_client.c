#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "net_client.h"
#include "ram_file_ctrl.h"
#include <net_infra.h>
#include "file_transfer_protocol.h"
#include <common_defs.h>

#define MAX_FILE_CHUNK_SIZE (1024) // 1 KB
#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     54322
#define TOTAL_SIZE (sizeof(data_chunk_protocol) + MAX_FILE_CHUNK_SIZE)

struct file_sender_client {
    ram_file_t *file_mngr;
    net_client_t *net_client;
};

int main(int argc, char *argv[]) {
    int rc;
    ssize_t file_size;

    // File-related variables
    ram_file_cfg_t file_config;
    const char *file_path;

    // Network client-related variables
    net_client_cfg_t net_client_cfg;

    // Struct for network and file objects
    struct file_sender_client file_sender = {0};

    // Data transfer variables
    uint8_t data_chunk_buffer[TOTAL_SIZE] = {0};
    data_chunk_protocol *data_chunk = (data_chunk_protocol *)data_chunk_buffer;
    net_client_data_t send_data;
    net_client_iov_t iov;

    net_infra_set_log_level(LOG_INFO);

    if (argc != 2) {
        NET_INFRA_LOG(LOG_ERROR,"Usage: %s <file_path>", argv[0]);
        return -EINVAL;
    }

    file_path = argv[1];

    rc = ram_file_exist(file_path);
    if (rc != 0) {
        NET_INFRA_LOG(LOG_ERROR,"Error accessing file");
        return rc;
    }

    file_size = ram_file_size_in_bytes(file_path);
    if (file_size < 0) {
        NET_INFRA_LOG(LOG_ERROR,"Error accessing file");
        return file_size;
    }

    NET_INFRA_LOG(LOG_INFO, "Starting main sender for file: %s, size: %zu bytes", file_path, file_size);

    // Set up the file management configuration
    file_config.max_file_size = file_size;
    file_config.delete_files = false;
    file_config.file_path = file_path;

    rc = ram_file_create(&file_config, &(file_sender.file_mngr));
    if (rc != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed creating ram_file");
        return rc;
    }

    // Create network client configuration and instance
    net_client_cfg.server_ip = SERVER_IP;
    net_client_cfg.server_port = SERVER_PORT;

    rc = net_client_create(&net_client_cfg, &(file_sender.net_client));
    if (rc != 0) {
        ram_file_destroy(file_sender.file_mngr);
        NET_INFRA_LOG(LOG_ERROR, "Failed creating client");
        return rc;
    }

    /* Setup I/O vector: total length includes header + payload */
    iov.data = (uint8_t *)data_chunk;
    send_data.iov_count = 1;
    send_data.iov = &iov;

    int file_read_err = 0;
    size_t file_offset = 0;

    while (file_offset < file_size) {
        size_t data_chunk_size = (file_size - file_offset < MAX_FILE_CHUNK_SIZE) ? (file_size - file_offset) : MAX_FILE_CHUNK_SIZE;
        file_read_err = ram_file_read(file_sender.file_mngr, file_offset, data_chunk->data, data_chunk_size);
        if (unlikely( file_read_err != 0)) {
            NET_INFRA_LOG(LOG_ERROR, "Error reading from file, %d: %s", file_read_err, strerror(abs(file_read_err)));
            ram_file_destroy(file_sender.file_mngr);
            net_client_destroy(file_sender.net_client);
            return file_read_err;          
        }

        data_chunk->flags = 0;
        data_chunk->offset = file_offset;
        iov.data_len = data_chunk_size + sizeof(*data_chunk);

        rc = net_client_send(file_sender.net_client, &send_data);
        if (rc != 0) {
            NET_INFRA_LOG(LOG_ERROR, "Failed sending data");
            ram_file_destroy(file_sender.file_mngr);
            net_client_destroy(file_sender.net_client);
            return rc;
        }

        file_offset += data_chunk_size;
    }

    ram_file_destroy(file_sender.file_mngr);
    net_client_destroy(file_sender.net_client);
    return 0;
}
