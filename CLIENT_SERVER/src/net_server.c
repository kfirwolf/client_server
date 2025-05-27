#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "net_server.h"
#include <net_infra.h>

#define SERVER_TIME_OUT_IN_SEC 5
#define SERVER_BUFFER_SIZE 10*1024 //(10kb)
#define SERVER_IP "127.0.0.1"

struct net_server_t {
    struct sockaddr_in sock_addr;   
    int sock_fd;
    pthread_t server_thread;
    atomic_int receive_data_flag;
    net_server_callback_t callback;
    void *user_ctx;
};

static void set_receive_data_flag(atomic_int *flag, int value) {
    atomic_store(flag, value);
}

static int get_receive_data_flag(atomic_int *flag) {
    return atomic_load(flag);
}

static int cleanup_server(net_server_t *server) {
    if (server == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "Server is not initialized");
        return -EINVAL;
    }
    if (server->sock_fd >= 0) {
        close(server->sock_fd);
    }
    free(server);
    return 0;
}

int net_server_create(net_server_cfg_t *server_cfg, net_server_t **server) {
    if (server_cfg == NULL || server == NULL) {
        return -EINVAL;
    }

    *server = (net_server_t *)malloc(sizeof(net_server_t));
    if (*server == NULL) {
        return -ENOMEM;
    }

    if (((*server)->sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NET_INFRA_LOG(LOG_ERROR, "Socket creation failed: %s", strerror(errno));
        cleanup_server(*server);
        return -errno;
    }

    int optval = 1;
    if (setsockopt((*server)->sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        NET_INFRA_LOG(LOG_ERROR, "setsockopt SO_REUSEADDR failed: %s", strerror(errno));
        cleanup_server(*server);
        return -errno;
    }

    (*server)->sock_addr.sin_family = AF_INET;
    (*server)->sock_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    (*server)->sock_addr.sin_port = htons(server_cfg->local_port);
    set_receive_data_flag(&((*server)->receive_data_flag), 0);

    struct timeval timeout = {SERVER_TIME_OUT_IN_SEC, 0};
    if (setsockopt((*server)->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        NET_INFRA_LOG(LOG_ERROR, "setsockopt SO_RCVTIMEO failed: %s", strerror(errno));
        cleanup_server(*server);
        return -errno;
    }

    if (bind((*server)->sock_fd, (const struct sockaddr *)&((*server)->sock_addr), sizeof((*server)->sock_addr)) < 0) {
        NET_INFRA_LOG(LOG_ERROR, "Bind failed: %s", strerror(errno));
        cleanup_server(*server);
        return -errno;
    }

    NET_INFRA_LOG(LOG_INFO, "Successfully created udp server");
    return 0;
}

static void *_net_server_recv_loop(void *server_data) {
    if (server_data == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "server_data is NULL");
        return NULL;
    }

    net_server_t *server = server_data;
    uint8_t buffer[SERVER_BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (get_receive_data_flag(&(server->receive_data_flag))) {
        ssize_t bytes_received = recvfrom(server->sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
        if (bytes_received > 0) {
            server->callback(client_addr.sin_addr.s_addr, client_addr.sin_port, buffer, bytes_received, server->user_ctx);
        } else if (bytes_received == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                NET_INFRA_LOG(LOG_INFO, "Receive timed out, continuing loop");
            } else {
                NET_INFRA_LOG(LOG_ERROR, "recvfrom failed: %s", strerror(errno));
                break;
            }
        }
    }

    return NULL;
}

int net_server_start(net_server_t *server, net_server_callback_t callback, void *user_ctx) {
    if (server == NULL || callback == NULL) {
        return -EINVAL;
    }

    set_receive_data_flag(&(server->receive_data_flag), 1);
    server->callback = callback;
    server->user_ctx = user_ctx;

    if (pthread_create(&server->server_thread, NULL, _net_server_recv_loop, server) != 0) {
        NET_INFRA_LOG(LOG_ERROR, "server thread creation failed: %s", strerror(errno));
        cleanup_server(server);
        return -errno;
    }
    NET_INFRA_LOG(LOG_INFO, "Successfully started UDP server");
    return 0;
}

int net_server_stop(net_server_t *server) {
    
    if (server == NULL || get_receive_data_flag(&(server->receive_data_flag)) == 0) {
        return -EINVAL;
    }

    set_receive_data_flag(&(server->receive_data_flag), 0);
    sleep(SERVER_TIME_OUT_IN_SEC);
    pthread_join(server->server_thread, NULL);
    NET_INFRA_LOG(LOG_DEBUG, "Successfully stoping udp server");
    return 0;
}

int net_server_destroy(net_server_t *server) {
    int rc = cleanup_server(server);
    if (rc != 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed net_server_destroy, error: %s", strerror(-rc));        
    }

    NET_INFRA_LOG(LOG_DEBUG, "Successfully cleanup udp server");
    return rc;
}
