#ifndef _HANDLE_REQUEST_H
#define _HANDLE_REQUEST_H 1

#include <netinet/in.h>

void handle_request(int sc, struct sockaddr_in client_addr, 
                    char *rootdir);

#endif