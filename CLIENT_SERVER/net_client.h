#include <stdint.h>
#include <string.h>

typedef struct net_client net_client; //forward declaration for compilation

typedef struct {
    char *ip;
    int port;
} net_client_cfg;    

typedef struct {
    uint8_t *data;
    size_t data_len;
} net_client_data;   



int net_client_create(net_client_cfg *cfg, net_client **client);
int net_client_send(net_client *client, net_client_data *data);
int net_client_recive(net_client *client);
int net_client_destroy(net_client *client);