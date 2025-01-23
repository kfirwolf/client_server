#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include "net_client.h"

typedef struct {
    struct sockaddr_in *sock_addr;
    int sock_fd;
    int time_out;
} net_client;


int net_client_create(net_client_cfg *cfg, net_client **client) {
    //add print here
    if (client == NULL || cfg == NULL) {
        return -1; // Client/cfg pointers are NULL
    }

    *client = (net_client *)malloc(sizeof(net_client));

    if (*client == NULL) {
        //add error message
        return -1;
    }

    // Create socket:
    (*client)->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if((*client)->sock_fd < 0){
        printf("Error while creating socket\n");
        free(*client);
        return -1;
    }

    printf("Socket created successfully\n");

    (*client)->sock_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    if ((*client)->sock_addr == NULL) {
        printf("Error: Unable to allocate memory for sockaddr_in\n");
        close((*client)->sock_fd);
        free(*client);
        return -1;
    }    

    // Set port and IP:
    (*client)->sock_addr->sin_family = AF_INET;
    (*client)->sock_addr->sin_port = htons(cfg->port);
    (*client)->sock_addr->sin_addr.s_addr = inet_addr(cfg->ip);

    //need to set timeout value 

    return 0;
}