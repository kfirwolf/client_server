#include <stdint.h>
#include <string.h>
#include <unistd.h>

typedef struct sockaddr sockaddr_t; //defined it instead of using #include <netinet/in.h> and using sockaddr_in (we will use the correct one in the c file)
typedef unsigned int socklen_t; //defined it instead of #include <sys/socket.h> here

typedef struct {
    char *ip;
    uint16_t local_port;
    int sock_fd;
    int time_out;
} cfg_t;


typedef void (*data_handler_t)(const uint8_t *data, size_t data_len, const struct sockaddr_t *src_addr, socklen_t addrlen); //callback for continues retrive of data


int net_server_start(cfg_t *server_cfg, data_handler_t *d_handler);
int net_server_stop(cfg_t *server_cfg);
