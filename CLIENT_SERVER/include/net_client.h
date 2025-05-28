#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <infra_log.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct net_client_t net_client_t; //for compilation and proper module design

typedef struct {
    const char *server_ip;
    uint16_t server_port;
} net_client_cfg_t;    

typedef struct {
    uint8_t *data; // Pointer to the data buffer
    size_t data_len; // Length of the data
} net_client_iov_t; // IO vector

typedef struct {
    net_client_iov_t *iov; // Pointer to an array of IO vectors
    size_t iov_count; // Number of IO vectors
} net_client_data_t;


int net_client_create(net_client_cfg_t *cfg, net_client_t **client);
int net_client_send(net_client_t *client, net_client_data_t *data);
int net_client_receive(net_client_t *client);
int net_client_destroy(net_client_t *client);

#ifdef __cplusplus
}
#endif