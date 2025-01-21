#include <stdint.h>
#include <string.h>

int net_server_create();
int net_server_start();
int net_server_stop();
int net_server_destroy();
int net_server_bind(int socketFD);
int net_server_unbind(int socketFD);