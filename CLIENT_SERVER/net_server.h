#include <stdint.h>
#include <string.h>

typedef struct {
    char *ip;
    int port;
    int sock_fd;
    int time_out;
} cfg_t;    

int net_server_start(cfg_t *cfg);
int net_server_stop();
int net_server_destroy();
