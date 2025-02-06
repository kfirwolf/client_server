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
#include "net_infra.h"


#define SERVER_TIME_OUT_IN_SEC 5
#define SERVER_BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"


struct net_server_t {
    struct sockaddr_in sock_addr;   
    int sock_fd;
    pthread_t server_thread;
    atomic_int  receive_data_flag;
    net_server_callback_t callback; // The callback function
    void *user_ctx;
};

static void set_receive_data_flag(atomic_int *flag, int value) {
    atomic_store(flag, value);
}

static int get_receive_data_flag(atomic_int *flag) {
    return atomic_load(flag);
}

static int cleanup_server(net_server_t *server) {
    if (server == NULL) { //change tabs to space not dot (tab indentation)
        NET_LOG(LOG_ERROR, "%s", "Server is not initialized");
        return -1;
    }

    // Close the socket to release the file descriptor
    if (server->sock_fd >= 0) {
        close(server->sock_fd);
    }

    // Free the allocated memory for the server struct
    free(server);

    return 0;
}

int net_server_create(net_server_cfg_t *server_cfg, net_server_t **server) {
    //add print here
    if (server_cfg == NULL || server == NULL) {
        return -1;
    }

    *server = (net_server_t *)malloc(sizeof(net_server_t));

    if (*server == NULL) {
        return -1;
    }

    if (((*server)->sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NET_LOG(LOG_ERROR, "%s","Socket creation failed");
        cleanup_server(*server);
        return -1;
    }

    // Enable the SO_REUSEADDR option (so we can reuse the socket after destorying it , when destory , the socket in wait state for a short period)
    int optval = 1;
    if (setsockopt((*server)->sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        NET_LOG(LOG_ERROR, "%s","setsockopt SO_REUSEADDR");        
        cleanup_server(*server);
        return -1;
    }

    // Filling server information
    (*server)->sock_addr.sin_family = AF_INET; // IPv4
    (*server)->sock_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    (*server)->sock_addr.sin_port = htons(server_cfg->local_port);
    set_receive_data_flag(&((*server)->receive_data_flag), 0);

    struct timeval timeout = {SERVER_TIME_OUT_IN_SEC, 0};
    if (setsockopt((*server)->sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        NET_LOG(LOG_ERROR, "%s", "setsockopt failed");        
        cleanup_server(*server);
        return -1;        
    }

        NET_LOG(LOG_DEBUG, "%s: %d", "socket_fd",(*server)->sock_fd);  

    // Bind the socket with the server address
    if (bind((*server)->sock_fd, (const struct sockaddr *)&((*server)->sock_addr), sizeof((*server)->sock_addr)) < 0) {
        NET_LOG(LOG_ERROR, "%s: %s", "bind failed",strerror(errno)); 
        cleanup_server(*server);
        return -1;
    }
        NET_LOG(LOG_DEBUG, "%s", "server created successfully");

    return 0; //use only from errono , the EXIT_SUCCESS....mostly use for application level
}

void *_net_server_recv_loop(void *server_data) {

    if (server_data == NULL) {
        NET_LOG(LOG_ERROR, "%s", "server_data = null");        
        return NULL;
    }

    net_server_t *server = server_data;
    uint8_t buffer[SERVER_BUFFER_SIZE]; 
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (get_receive_data_flag(&(server->receive_data_flag))) {

        ssize_t bytes_received = recvfrom(server->sock_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);

        if (bytes_received > 0) {
            // threre is a potential issue here , we are the buffer as a pointer here but the callback implementation function must take this data and save it or process it before the next callback is called , because the data will be overwritten     
            server->callback(client_addr.sin_addr.s_addr, client_addr.sin_port, buffer, bytes_received, server->user_ctx);
        }
        else if (bytes_received == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout occurred, no data received
                NET_LOG(LOG_INFO, "%s", "Receive timed out, continuing loop"); 
            }
            else {
                // Handle other errors
                NET_LOG(LOG_ERROR, "%s: %s", "recvfrom failed",strerror(errno));
                break;
            }
        }
        else if (bytes_received == 0) {
            // Unlikely for UDP, but handle gracefully
            NET_LOG(LOG_WARNING, "%s", "Received 0 bytes, continuing loop");
        }
    }

    return NULL;
}

int net_server_start(net_server_t *server, net_server_callback_t callback, void *user_ctx) {

    if (server == NULL || callback == NULL) {
        NET_LOG(LOG_ERROR, "%s", "failed to start server");        
        return -1;
    }

    set_receive_data_flag(&(server->receive_data_flag), 1);

    server->callback = callback;
    server->user_ctx = user_ctx;

    //thread creation
    if (pthread_create(&server->server_thread, NULL, _net_server_recv_loop, server) != 0) {
        NET_LOG(LOG_ERROR, "%s", "server failed main thread");        
        server->receive_data_flag = 0;
        cleanup_server(server);
        return -1;
    }
        NET_LOG(LOG_DEBUG, "%s", "server start successfully");
    return 0;
}

int net_server_stop(net_server_t *server) {

    if (server == NULL || get_receive_data_flag(&(server->receive_data_flag)) == 0) {
        NET_LOG(LOG_ERROR, "%s", "failed to stop server");        
        return -1;
    }

    set_receive_data_flag(&(server->receive_data_flag), 0);
    // Join the server thread after timeout (max time reveive will be stuck on the loop)
    sleep(SERVER_TIME_OUT_IN_SEC);
    pthread_join(server->server_thread, NULL);

    return 0;
}

int net_server_destroy(net_server_t *server) {
    NET_LOG(LOG_DEBUG, "%s", "Destroying server...");
    return cleanup_server(server);
}
