#define _POSIX_C_SOURCE 200809L //compiler will expose the POSIX definitions like sigaction 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include "net_server.h"
#include "ram_file_ctrl.h"
#include "net_infra.h"
#include "file_transfer_protocol.h"

#define MAX_FILE_SIZE (1 * 1024 * 1024)  // 1 MB
#define MAX_FILE_BUFFER_SIZE 1024 //1 KB
#define FILE_PATH "/tmp/received_ram_file_ctrl.bin"
#define SERVER_PORT     54322

net_server_t *global_net_server = NULL;
ram_file_t *global_file_mngr = NULL;
volatile sig_atomic_t stop_requested = 0;

void handle_signal(int signum) {
    stop_requested = 1;
}

static void Received_data_callback(uint32_t client_ip, uint16_t client_port, void *data, size_t data_size, void *ctx) {
    if (data_size < sizeof(data_chunk_protocol)) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal data");
        return;
    }

    size_t file_data_size = data_size - sizeof(data_chunk_protocol);
    if (file_data_size > MAX_FILE_BUFFER_SIZE) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal file data_size: %zu", file_data_size);
        return;
    }

    data_chunk_protocol *data_chunk = (data_chunk_protocol *)data;
    ram_file_t *file_manager = (ram_file_t *)ctx;


    NET_INFRA_LOG(LOG_INFO, "File data_size: %zu", file_data_size);

    if(ram_file_write(file_manager, data_chunk->offset, data_chunk->data, file_data_size) != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed writing to file");
        return;
    }
}

int main() {
    net_infra_set_log_level(LOG_INFO);
    NET_INFRA_LOG(LOG_INFO, "Starting main receiver");

    // Register signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_signal;       // Set our handler function
    sa.sa_flags = 0;                     // No special flags
    sigemptyset(&sa.sa_mask);            // No signals blocked during handler execution

    // Register the handler for SIGINT and SIGTERM (will use to gracfully close process after kill command like ctrl c or kill -15 pid)
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGINT");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("Error: cannot handle SIGTERM");
        exit(EXIT_FAILURE);
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

    global_file_mngr = file_mngr;
    NET_INFRA_LOG(LOG_INFO, "Successfully created ram_file");

    net_server_t *net_server;
    net_server_cfg_t net_server_cfg;
    net_server_cfg.local_port = SERVER_PORT;

    if (net_server_create(&net_server_cfg, &net_server) != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed creating udp server");
        return -1;        
    }

    global_net_server = net_server;
    NET_INFRA_LOG(LOG_INFO, "Successfully created udp server"); 

    if (net_server_start(net_server, Received_data_callback, (void *)file_mngr) != 0) {
        NET_INFRA_LOG(LOG_ERROR, "UDP server failed");
        net_server_stop(net_server);
        return -1;        
    }
    NET_INFRA_LOG(LOG_INFO, "Successfully starting UDP server");

    // Main loop: wait until a signal is received
    while (!stop_requested) {
        pause();

    }

    // Cleanup resources
    NET_INFRA_LOG(LOG_INFO, "Shutdown signal received. Cleaning up resources...");
    if (global_net_server) {
        net_server_stop(global_net_server);
        net_server_destroy(global_net_server);
    }

    if (global_file_mngr) {
        ram_file_destroy(global_file_mngr); // Assuming this function exists
    }

    NET_INFRA_LOG(LOG_INFO, "Gracefully shutdown server and cleaned up resources.");

    return 0;
}
