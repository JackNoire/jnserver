#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define MAXLINE 1024 //一行的最大长度

static char httpline[MAXLINE];

enum {
    ERROR_MSG_404 = 0,
    ERROR_MSG_414,
    ERROR_MSG_500,
    ERROR_MSG_400,
    ERROR_MSG_TOTAL
};
static char *error_msg[ERROR_MSG_TOTAL] = {
    "HTTP/1.1 404 Not Found\r\n"
    "Content-length: 95\r\n\r\n"
    "<h1>404 Not Found</h1>"
    "<p>The requested URL was not found on this server.</p>"
    "<hr><i>jnserver</i>",

    "HTTP/1.1 414 URI Too Long\r\n"
    "Content-length: 81\r\n\r\n"
    "<h1>414 URI Too Long</h1>"
    "<p>The requested URI is too long.</p>"
    "<hr><i>jnserver</i>",

    "HTTP/1.1 500 Internal Server Error\r\n"
    "Content-length: 81\r\n\r\n"
    "<h1>500 Internal Server Error</h1>"
    "<p>An error occurred :-(</p>"
    "<hr><i>jnserver</i>",

    "HTTP/1.1 400 Bad Request\r\n"
    "Content-length: 63\r\n\r\n"
    "<h1>400 Bad Request</h1>"
    "<p>Syntax error.</p>"
    "<hr><i>jnserver</i>"
};
#define SEND_ERROR_MSG(code) printf("[Reply] %s:%d error code " #code "\n", \
        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); \
        writen(sc, error_msg[ERROR_MSG_##code], strlen(error_msg[ERROR_MSG_##code]));

/*
 * Definition of mime_types and get_mime_type taken from:
 * https://github.com/shenfeng/tiny-web-server
 */
typedef struct {
    const char *extension;
    const char *mime_type;
} mime_map;
mime_map meme_types [] = {
    {".css", "text/css"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".ico", "image/x-icon"},
    {".js", "application/javascript"},
    {".pdf", "application/pdf"},
    {".mp4", "video/mp4"},
    {".png", "image/png"},
    {".svg", "image/svg+xml"},
    {".xml", "text/xml"},
    {NULL, NULL},
};
char *default_mime_type = "text/plain";
static const char* get_mime_type(char *filename){
    char *dot = strrchr(filename, '.');
    if(dot){ // strrchar Locate last occurrence of character in string
        mime_map *map = meme_types;
        while(map->extension){
            if(strcmp(map->extension, dot) == 0){
                return map->mime_type;
            }
            map++;
        }
    }
    return default_mime_type;
}

ssize_t writen(int fd, void *usrbuf, size_t n){
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0){
        if ((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)  /* interrupted by sig handler return */
                nwritten = 0;    /* and call write() again */
            else
                return -1;       /* errorno set by write() */
        }
        nleft -= nwritten;
        bufp += nwritten;
        if (nleft > 0) {
            usleep(50000);
        }
    }
    return n;
}

/*
 * 读取http请求中的一行，保存到httpline中
 * 返回读取的字符数，返回-1表示read发生错误，-2表示字符数超出上限
 */
static ssize_t read_httpline(int sc) {
    for (int n = 0; n < MAXLINE - 1; n++) {
        ssize_t readCount = read(sc, &httpline[n], 1);
        if (readCount == 1) { //正常情况，读取一个字符
            if (httpline[n] == '\n' || httpline[n] == '\r') {
                if (n == 0) { //一开始就读取'\n'或'\r'
                    n--;
                    continue;
                } else { //一行已读取完毕
                    httpline[n] = '\0';
                    return n;
                }
            }
        } else if (readCount == 0) { //读到EOF
            httpline[n] = '\0';
            return n;
        } else { //read发生错误
            return -1;
        }
    }
    return -2; //字符数超出上限
}

/*
 * 对str进行URL解码，返回1表示解码成功
 */
static int url_decode(char *str, int maxlen) {
    char *result = (char *)malloc(maxlen);
    if (NULL == result) {
        printf("malloc error\n");
        return 0;
    }

    char *ptr = result;
    char code[3] = {0};
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] != '%') {
            *ptr = str[i];
            ptr++;
        } else {
            code[0] = str[i+1];
            code[1] = str[i+2];
            *ptr = (char)strtoul(code, NULL, 16);
            i = i + 2;
        }
    }
    *ptr = '\0';
    strcpy(str, result);

    free(result);
    return 1;
}

/*
 * 向客户端发送文件内容
 */
static void send_file_to_client(int sc, int fd, char *filepath, off_t filesize) {
    char buf[1024];
    sprintf(buf,
            "HTTP/1.1 200 OK\r\n"
            "Content-length: %ld\r\n"
            "Content-type: %s\r\n\r\n",
            filesize, get_mime_type(filepath));
    ssize_t count = writen(sc, buf, strlen(buf));
    if (count < 0) {
        printf("write error, %s", strerror(errno));
        return;
    }
    off_t offset = 0;
    while (offset < filesize) {
        usleep(50000); //如果不延时会导致连接重置
        if (sendfile(sc, fd, &offset, filesize - offset) <= 0) {
            printf("sendfile error.\n");
            break;
        }
        printf("Sending %s: %ld/%ld\n", filepath, offset, filesize);
    }
}

/*
 * 将目录文件信息发送给客户端
 */
static void send_dir_to_client(int sc, int fd, char *filepath, char *uri, struct sockaddr_in client_addr) {
    struct stat sbuf;
    char *indexpath = (char *)malloc(strlen(filepath) + 30);
    if (NULL == indexpath) {
        printf("malloc error.\n");
        SEND_ERROR_MSG(500);
        return;
    }
    strcpy(indexpath, filepath);
    strcat(indexpath, "/index.html");
    printf("%s\n", indexpath);
    int tmpfd = open(indexpath, O_RDONLY, 0);
    if (tmpfd > 0) { //尝试在文件夹中搜索index.html，如果存在则向客户端发送index.html
        fstat(tmpfd, &sbuf); //获取文件类型
        if (S_ISREG(sbuf.st_mode)) { //普通文件
            send_file_to_client(sc, tmpfd, indexpath, sbuf.st_size);
            close(tmpfd);
            free(indexpath);
            return;
        }
        close(tmpfd);
    }
    free(indexpath);
    //没有index.html，生成Index of页面
    char htmlbuf[2048];
    sprintf(htmlbuf, "<h1>Index of %s</h1>", uri);
    DIR *dp = opendir(filepath);
    if (NULL == dp) {
        printf("opendir error.\n");
        SEND_ERROR_MSG(500);
        return;
    }
    struct dirent *files;
    char tmpfilepath[1024];
    char tmpuri[1024];
    char printfilename[1024];
    while((files=readdir(dp)) != NULL) {
        if (!strcmp(files->d_name,"."))
            continue;
        if (!strcmp(files->d_name,"..")) {
            sprintf(htmlbuf + strlen(htmlbuf), "<a href=\"../\">../</a><br>");
            continue;
        }
        strcpy(tmpfilepath, filepath);
        strcat(tmpfilepath, "/");
        strcat(tmpfilepath, files->d_name);
        strcpy(tmpuri, uri);
        if (tmpuri[strlen(tmpuri) - 1] != '/') {
            strcat(tmpuri, "/");
        }
        strcat(tmpuri, files->d_name);
        strcpy(printfilename, files->d_name);
        if (stat(tmpfilepath, &sbuf) == -1) {
            printf("stat error.\n");
        } else if (S_ISDIR(sbuf.st_mode)) {
            strcat(printfilename, "/");
        }
        sprintf(htmlbuf + strlen(htmlbuf), 
                "<a href=\"%s\">%s</a><br>", 
                tmpuri, printfilename);
    }
    char buf[8192];
    sprintf(buf, 
            "HTTP/1.1 200 OK\r\n"
            "Content-length: %ld\r\n"
            "Content-type: text/html\r\n\r\n",
            strlen(htmlbuf));
    strcat(buf, htmlbuf);
    writen(sc, buf, strlen(buf));
}


/*
 * 处理客户端http请求
 */
void handle_request(int sc, struct sockaddr_in client_addr, char *rootdir) {
    char method[MAXLINE], uri[MAXLINE];

    ssize_t count = read_httpline(sc); //读取一行http请求
    switch(count) {
    case 0: {
        printf("No data from HTTP request.\n");
        SEND_ERROR_MSG(500);
        return;
    }
    case -1: {
        printf("read error, %s\n", strerror(errno));
        SEND_ERROR_MSG(500);
        return;
    }
    case -2: {
        printf("The maximum number of characters in a line is exceeded.\n");
        SEND_ERROR_MSG(414);
        return;
    }
    }

    count = sscanf(httpline, "%s%s", method, uri); //获取method和uri
    if (count != 2 || uri[0] != '/') {
        printf("syntax error\n");
        SEND_ERROR_MSG(400);
        return;
    }

    char *param = NULL;
    for (int i = 0; uri[i] != '\0'; i++) {
        if (uri[i] == '?') {
            uri[i] = '\0';
            param = &uri[i+1]; //如果出现'?'，则后面跟着的就是参数
        }
    }

    char *filepath = (char *)malloc(strlen(rootdir) + MAXLINE * 2);
    if (NULL == filepath) {
        printf("malloc error\n");
        SEND_ERROR_MSG(500);
        return;
    }
    strcpy(filepath, rootdir);
    strcat(filepath, uri); //拼接根目录和URI得到文件路径
    if (!url_decode(filepath, strlen(rootdir) + MAXLINE * 2)) { //URL解码
        printf("url_decode error\n");
        SEND_ERROR_MSG(500);
        goto exit;
    }
    printf("[Request] %s:%d %s %s\n", inet_ntoa(client_addr.sin_addr), 
            ntohs(client_addr.sin_port), method, filepath);
    int fd = open(filepath, O_RDONLY, 0); //尝试open查看文件是否存在
    if (fd <= 0) {
        SEND_ERROR_MSG(404);
        goto exit;
    } else {
        if (!strcmp(method, "GET")) {
            struct stat sbuf;
            fstat(fd, &sbuf); //获取文件类型
            if (S_ISREG(sbuf.st_mode)) { //普通文件
                send_file_to_client(sc, fd, filepath, sbuf.st_size);
            } else if (S_ISDIR(sbuf.st_mode)) { //目录文件
                send_dir_to_client(sc, fd, filepath, uri, client_addr);
            } else {
                printf("Unknown file type.\n");
                SEND_ERROR_MSG(400);
            }
        } else {
            printf("Only support GET method.\n");
            SEND_ERROR_MSG(500);
        }
    }
    close(fd);

exit:
    free(filepath);
}