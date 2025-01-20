#include <net_common.h>

int clientCreateSocket(char *ip, int port);

int clientSend(int socketfd, char *req, short reqLen);

int clientRecive(int socketfd, char *resp, short respLen);

int clientClose();