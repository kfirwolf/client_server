#include <stdint.h>
#include <string.h>

typedef struct {
    char *ip;
    int port;
    int sock_fd;
    int time_out;
} cfg_t;    

typedef struct {
    uint8_t *data;
    int data_len;
} data_t;

int net_client_create(cfg_t *cfg);
int net_client_send(cfg_t *cfg, const data_t *data_s);
int net_client_recive(cfg_t *cfg, data_t *data_s);
int net_client_destroy(cfg_t *cfg);