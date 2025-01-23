#include <stdint.h>
#include <string.h>

typedef struct net_client net_client; //for compilation and proper module design

typedef struct {
    char *ip;
    int port;
} net_client_cfg_t;    

typedef struct {
    uint8_t *data; // Pointer to the data buffer
    size_t data_len; // Length of the data
} net_client_iov_t; // IO vector

typedef struct {
    net_client_iov_t *iov; // Pointer to an array of IO vectors
    size_t iov_count; // Number of IO vectors
} net_client_data_t;


int net_client_create(net_client_cfg_t *cfg, net_client **client);
int net_client_send(net_client *client, net_client_data_t *data);
int net_client_recive(net_client *client);
int net_client_destroy(net_client *client);