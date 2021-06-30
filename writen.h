#ifndef _WRITEN_H
#define _WRITEN_H 1

#include <unistd.h>
ssize_t writen(int fd, void *usrbuf, size_t n);

#endif