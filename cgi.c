#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#include "writen.h"
#include "handle_request.h"

void send_php_to_client(int sc, char *filepath, char *param) {
    int piped[2];
    pid_t pid;

    if (pipe(piped) < 0) {
        printf("pipe error, %s\n", strerror(errno));
        return;
    }

    if ((pid = fork()) < 0) {
        printf("fork error, %s\n", strerror(errno));
        close(piped[0]);
        close(piped[1]);
        return;
    } else if (pid == 0) { //子进程
        close(piped[0]);
        dup2(piped[1], STDOUT_FILENO); //stdout重定向到piped[1]管道
        int paramCount = 0; //参数个数
        if (param != NULL) {
            paramCount++;
            for (char *p = param; *p != '\0'; p++) {
                if (*p == '&') {
                    *p = '\0';
                    paramCount++;
                }
            }
        }
        //char *argv[5] = {"/usr/bin/php-cgi", "-f", filepath, "num=12", NULL};
        char **argv = (char **)malloc(sizeof(char *) * (paramCount + 4));
        argv[0] = "/usr/bin/php-cgi";
        argv[1] = "-f";
        argv[2] = filepath;
        for (int i = 0; i < paramCount; i++) {
            argv[i + 3] = param;
            while (*param != '\0') {
                param++;
            }
            param++; //param指针移动到下一个参数
        }
        argv[paramCount + 3] = NULL;
        execve("/usr/bin/php-cgi", argv, NULL);
        printf("execve failed, %s\n", strerror(errno));
        free(argv);
        _exit(0);
    } else {
        close(piped[1]);

        char httpReply[8192];
        char *htmlbuf = &httpReply[200];
        ssize_t count = read(piped[0], htmlbuf, 7900);
        if (count > 0) {
            sprintf(httpReply,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-length: %ld\r\n"
                    "Content-type: text/html; charset=UTF-8\r\n\r\n", count);
            char *httpHdrEnd = httpReply + strlen(httpReply);
            for (int i = 0; i < count; i++) {
                httpHdrEnd[i] = htmlbuf[i];
            }
            httpHdrEnd[count] = '\0';
            writen(sc, httpReply, strlen(httpReply));
        } else {
            writen(sc, error_msg[ERROR_MSG_400], strlen(error_msg[ERROR_MSG_400]));
        }
        
        close(piped[0]);
    }
}