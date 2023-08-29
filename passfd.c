/* This source code has been inspired by the examples of the Book:
 * UNIX Network Programming Volume 1, Third Edition
 * https://github.com/unpbook/unpv13e/tree/master
 * files lib/send_fd.c and lib/write_fd.c */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

ssize_t write_fd(const int fd, const void *const ptr, const size_t nbytes, const int sendfd) {
    struct msghdr msg;
    struct iovec iov[1];

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr *cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int *)CMSG_DATA(cmptr)) = sendfd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = (void *)ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    return sendmsg(fd, &msg, 0);
}

ssize_t read_fd(const int fd, void *const ptr, const size_t nbytes, int *const recvfd) {
    struct msghdr msg;
    struct iovec iov[1];
    ssize_t n;

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;
    struct cmsghdr *cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if ((n = recvmsg(fd, &msg, 0)) <= 0) {
        return n;
    }

    if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL && cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
        if (cmptr->cmsg_level != SOL_SOCKET) {
            fprintf(stderr, "control level != SOL_SOCKET");
						*recvfd = -1;
						return n;
        }

        if (cmptr->cmsg_type != SCM_RIGHTS) {
            fprintf(stderr, "control type != SCM_RIGHTS");
						*recvfd = -1;
						return n;
        }

        *recvfd = *((int *)CMSG_DATA(cmptr));
    } else {
        *recvfd = -1; /* descriptor was not passed */
    }

    return n;
}
