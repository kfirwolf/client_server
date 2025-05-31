#define _POSIX_C_SOURCE 200809L //compiler will expose the POSIX definitions like sigaction 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <signal.h>
#include "net_server.h"
#include "ram_file_ctrl.h"
#include <infra_log.h>
#include "file_transfer_protocol.h"
#include <common_defs.h>

#define MAX_FILE_SIZE (1 * 1024 * 1024)  // 1 MB
#define MAX_FILE_BUFFER_SIZE 1024 //1 KB
#define FILE_PATH "/tmp/received_ram_file_ctrl.bin"
#define SERVER_PORT     54322

struct file_sender_server {
    ram_file_t *file_mngr;
    net_server_t *net_server;
};

struct file_sender_server file_sender = {0};
volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int signum) {
    stop_requested = 1;
}

static void register_signal_handlers(void) {
    // Register signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_signal;       // Set our handler function
    sa.sa_flags = 0;                     // No special flags
    sigemptyset(&sa.sa_mask);            // No signals blocked during handler execution

    // Register the handler for SIGINT and SIGTERM (will use to gracfully close process after kill command like ctrl c or kill -15 pid)
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        NET_INFRA_LOG(LOG_ERROR, "Error: cannot handle SIGINT");
        exit(EXIT_FAILURE);
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        NET_INFRA_LOG(LOG_ERROR, "Error: cannot handle SIGTERM");
        exit(EXIT_FAILURE);
    }
}

static void server_data_to_file_callback(uint32_t client_ip, uint16_t client_port, void *data, size_t data_size, void *ctx) {

    int rc = 0;
    data_chunk_protocol *data_chunk = (data_chunk_protocol *)data;
    ram_file_t *file_manager = (ram_file_t *)ctx;

    if (unlikely(data_size < sizeof(data_chunk_protocol))) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal data_size , data_size: %zu < sizeof(data_chunk_protocol)", data_size);
        return;
    }

    size_t file_data_size = data_size - sizeof(data_chunk_protocol);
    if (file_data_size > MAX_FILE_BUFFER_SIZE) {
        NET_INFRA_LOG(LOG_ERROR, "Illegal file data_size: %zu", file_data_size);
        return;
    }

    NET_INFRA_LOG(LOG_DEBUG, "File data_size: %zu", file_data_size);
    rc = ram_file_write(file_manager, data_chunk->offset, data_chunk->data, file_data_size);
    if(rc != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed writing to file, error: %s", strerror(-rc));
        return;
    }
}

static void app_teardown() {
    net_server_stop(file_sender.net_server);
    net_server_destroy(file_sender.net_server);
    ram_file_destroy(file_sender.file_mngr);
}

static int app_setup(void) {
    int rc = 0;
    ram_file_cfg_t file_config;
    net_server_cfg_t net_server_cfg;

    net_infra_set_log_level(LOG_INFO);
    NET_INFRA_LOG(LOG_INFO, "Starting main receiver");

    // Register signal handlers
    register_signal_handlers();

    // Set up RAM file configuration
    file_config.max_file_size = MAX_FILE_SIZE;
    file_config.delete_files = false;
    file_config.file_path = FILE_PATH;

    // Create the RAM file
    rc = ram_file_create(&file_config, &(file_sender.file_mngr));
    if (rc != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed creating ram_file, error: %s", strerror(-rc));
        return rc;
    }

    // Set up network server configuration
    net_server_cfg.local_port = SERVER_PORT;

    // Create the network server
    rc = net_server_create(&net_server_cfg, &(file_sender.net_server));
    if (rc != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed creating udp server, error: %s", strerror(-rc));
        return rc;
    }

    return 0;
}

static int app_run(void) {
    int rc = net_server_start(file_sender.net_server, server_data_to_file_callback, (void *)file_sender.file_mngr);
    if (rc != 0) {
        NET_INFRA_LOG(LOG_ERROR, "UDP server failed to start, error: %s", strerror(-rc));
    }
    return rc;
}

int main() {
    int rc = app_setup();
    if (rc != 0) {
        app_teardown();
        return rc;
    }

    rc = app_run();
    if (rc != 0) {
        app_teardown();
        return rc;
    }

    // Main loop: wait until a signal is received
    while (!stop_requested) {
        pause();
    }

    NET_INFRA_LOG(LOG_INFO, "Shutdown signal received. Cleaning up resources...");
    app_teardown();
    return 0;
}



