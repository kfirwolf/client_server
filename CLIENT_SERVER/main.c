#include <stdio.h>
#include "net_server.h"
#include "net_client.h"


int main() {


    //creating client:

    net_client *client;
    net_client_cfg_t cfg;
    
    cfg.ip = "192.168.1.10";
    cfg.port = 10808;

    net_client_create(&cfg, &client);
}