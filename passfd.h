#ifndef PASSFD_H
#define PASSFD_H
#include <stdio.h>

ssize_t write_fd(const int fd, const void *const ptr, const size_t nbytes, const int sendfd);
ssize_t read_fd(const int fd, void *const ptr, const size_t nbytes, int *const recvfd);

#endif
