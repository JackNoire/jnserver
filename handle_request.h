#ifndef _HANDLE_REQUEST_H
#define _HANDLE_REQUEST_H 1

#include <netinet/in.h>

enum {
    ERROR_MSG_404 = 0,
    ERROR_MSG_414,
    ERROR_MSG_500,
    ERROR_MSG_400,
    ERROR_MSG_TOTAL
};
extern char *error_msg[ERROR_MSG_TOTAL];

void handle_request(int sc, struct sockaddr_in client_addr, 
                    char *rootdir);

#endif