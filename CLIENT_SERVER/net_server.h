#include <stdint.h>
#include <string.h>
#include <unistd.h>

typedef struct net_server net_server; //forward declaration for compilation

typedef struct {
    uint16_t local_port;
    int time_out;
} net_server_cfg;


int net_server_create(net_server_cfg *server_cfg, net_server **server);
int net_server_start(net_server *server);
int net_server_stop(net_server *server);
int net_server_destroy(net_server *server);
