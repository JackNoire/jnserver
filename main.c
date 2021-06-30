#include <arpa/inet.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <bits/getopt_posix.h>
#include <sys/time.h>

#include "handle_request.h"

int main(int argc, char **argv) {
    int ch;
    char *rootdir = ".";
    int port = 8910;
    //遍历命令行，获取服务器根目录和端口号
    while ((ch = getopt(argc, argv, "r:p:")) != -1) {
        switch (ch) {
        case 'r': {
            rootdir = optarg;
            break;
        }
        case 'p': {
            port = atoi(optarg);
            if (port < 10 || port > 20000) {
                printf("Port number range 10~20000\n");
                return -1;
            }
            break;
        }
        case '?':
            printf("Usage: jnserver [-r rootdir] [-p portnumber]\n");
            return -1;
        default:
            abort();
        }
    }
    printf("rootdir: %s, port number: %d\n", rootdir, port);

    signal(SIGPIPE, SIG_IGN); //忽略SIGPIPE信号

    int ss, sc; //ss为服务器socket描述符，sc为客户端socket描述符
    struct sockaddr_in server_addr; //服务器地址
    struct sockaddr_in client_addr; //客户端地址
    ss = socket(AF_INET, SOCK_STREAM, 0); //创建服务器套接字
    if (ss < 0) {
        printf("socket error, %s\n", strerror(errno));
        return -1;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    //绑定地址
    if (bind(ss, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("bind error, %s\n", strerror(errno));
        close(ss);
        return -1;
    }
    //设置侦听队列长度
    if (listen(ss, 10) < 0) {
        printf("listen error, %s\n", strerror(errno));
        close(ss);
        return -1;
    }
    
    while(1) {
        socklen_t addrlen = sizeof(client_addr);
        sc = accept(ss, (struct sockaddr *)&client_addr, &addrlen);
        if (sc < 0) {
            continue;
        }
        
        if (fork() > 0) {
            close(sc);
        } else {
            close(ss);
            handle_request(sc, client_addr, rootdir);
            close(sc);
            return 0;
        }
    }

    return 0;
}