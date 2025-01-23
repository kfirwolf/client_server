#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "net_server.h"
#include <stdlib.h>

typedef struct  {
    struct sockaddr_in *sock_addr;
    int sock_fd;
} net_server;


int net_server_create(net_server_cfg *server_cfg, net_server **server) {
    //add print here
    if (server_cfg == NULL || server == NULL) {
        return -1;
    }

    net_server *server = (net_server *)malloc(sizeof(net_server));

    if (*server == NULL) {
        //add error message
        return -1;
    }

    if (((*server)->sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        free(*server);
        return -1;
    }

    (*server)->sock_addr = malloc(sizeof(struct sockaddr_in));

    if ((*server)->sock_addr == NULL) {
        printf("Error: Unable to allocate memory for sockaddr_in\n");
        close((*server)->sock_fd);
        free(*server);
        return -1;
    }        


    memset((*server)->sock_addr, 0, sizeof((*server)->sock_addr));

    // Filling server information
    (*server)->sock_addr->sin_family = AF_INET; // IPv4
    (*server)->sock_addr->sin_addr.s_addr = INADDR_LOOPBACK; // Loopback IP address 127.0.0.1
    (*server)->sock_addr->sin_port = htons(server_cfg->local_port);


    // Bind the socket with the server address
    if (bind((*server)->sock_fd, (const struct sockaddr *)(*server)->sock_addr, sizeof((*server)->sock_addr)) < 0) {
        perror("bind failed");
        close((*server)->sock_fd);
        //need to free memory
        exit(EXIT_FAILURE);
    }

    return 0;
}


int net_server_start(net_server *server) {
    //add check for null
    printf("UDP server listening on loopback address 127.0.0.1:%hu\n", server->sock_addr->sin_port);

}

