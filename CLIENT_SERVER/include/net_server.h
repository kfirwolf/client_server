#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct net_server_t net_server_t; //forward declaration for compilation
typedef void (*net_server_callback_t)(uint32_t client_ip, uint16_t client_port, void *data, size_t data_size, void *ctx);

typedef struct {
    uint16_t local_port;
} net_server_cfg_t;

int net_server_create(net_server_cfg_t *server_cfg, net_server_t **server);
int net_server_start(net_server_t *server, net_server_callback_t callback, void *user_ctx);
int net_server_stop(net_server_t *server);
int net_server_destroy(net_server_t *server);

#ifdef __cplusplus
}
#endif
