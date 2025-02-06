#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include "net_client.h"

struct net_client_t {
    struct sockaddr_in sock_addr;
    int sock_fd;
    int time_out;
};

int net_client_create(net_client_cfg_t *cfg, net_client_t **client) {
    if (client == NULL || cfg == NULL) {
        return -EINVAL; // Invalid argument
    }

    *client = (net_client_t *)calloc(1, sizeof(net_client_t));
    if (*client == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "Memory allocation failed");
        return -ENOMEM; // Out of memory
    }

    (*client)->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if ((*client)->sock_fd < 0) {
        NET_INFRA_LOG(LOG_ERROR, "Error while creating socket: %s", strerror(errno));
        free(*client);
        return -errno; // Return specific system error
    }

    NET_INFRA_LOG(LOG_INFO, "Client socket created successfully");

    (*client)->sock_addr.sin_family = AF_INET;
    (*client)->sock_addr.sin_port = htons(cfg->server_port);
    if (inet_aton((const char *)cfg->server_ip, &(*client)->sock_addr.sin_addr) == 0) {
        NET_INFRA_LOG(LOG_ERROR, "Invalid IP address");
        free(*client);
        return -EINVAL; // Invalid argument
    }

    return 0;
}

int net_client_destroy(net_client_t *client) {
    if (client == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "Client is not initialized");
        return -EINVAL; // Invalid argument
    }

    if (client->sock_fd >= 0) {
        close(client->sock_fd);
    }

    free(client);
    return 0;
}

int net_client_send(net_client_t *client, net_client_data_t *data) {
    if (client == NULL || data == NULL) {
        NET_INFRA_LOG(LOG_ERROR, "Can't send data to server: invalid parameters");
        return -EINVAL; // Invalid argument
    }

    NET_INFRA_LOG(LOG_DEBUG, "Client sending data:");
    net_infra_log_data(data->iov->data, data->iov->data_len);

    ssize_t sent_bytes = sendto(client->sock_fd, (const void *)(data->iov->data), data->iov->data_len, 0, (const struct sockaddr *)(&client->sock_addr), sizeof(client->sock_addr));
    if (sent_bytes < 0) {
        NET_INFRA_LOG(LOG_ERROR, "Failed sending data to server: %s", strerror(errno));
        return -errno; // Return specific system error
    }

    return 0;
}
