#include "unpthread.h"
#include "unp.h"

void  Pthread_create(pthread_t * tid, const pthread_attr_t * attr,
                       THREAD_FUNC * func, void *arg)
{
    int n;
    if( (n = pthread_create(tid, attr, func, arg)) != 0)
    {
        errno = n;
        err_quit("pthread create error");
    }
}

void Pthread_detach(pthread_t tid)
{
    int n;
    if((n = pthread_detach(tid)) == 0)
    {
        return;
    }
    else
    {
        errno = n;
        err_quit("pthread detack error");
    }
}

void Pthread_mutex_lock(pthread_mutex_t *mptr)
{
    int n;
    if((n = pthread_mutex_lock(mptr))  == 0)
    {
        return;
    }
    else
    {
        errno = n;
        err_quit("pthread_mutex_lock lock");
    }
}

void Pthread_mutex_unlock(pthread_mutex_t *mptr)
{
    int n;
    if((n = pthread_mutex_unlock(mptr))  == 0)
    {
        return;
    }
    else
    {
        errno = n;
        err_quit("pthread_mutex_unlock lock");
    }
}

int Select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * execpfds, struct timeval *timeout)
{
    int n;
    for(;;)
    {
        if((n = select(nfds, readfds, writefds, execpfds, timeout)) < 0)
        {
            if(errno == EINTR) continue;
            else err_quit("select error");
        }
        else break;
    }
    return n; //can return 0 on timeout
}

ssize_t write_fd(int fd, void *ptr, size_t nbytes, int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];

    union{
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    }control_un;
    struct cmsghdr *cmptr;

    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_len = CMSG_LEN(sizeof(int));
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    *((int *) CMSG_DATA(cmptr)) = sendfd;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;

    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    return (sendmsg(fd, &msg, 0));
}


ssize_t Write_fd(int fd, void *ptr, size_t nbytes, int sendfd)
{
    ssize_t n;
    if((n = write_fd(fd, ptr, nbytes, sendfd)) < 0)
    {
        err_quit("write fd error");
    }
    return n;
}


ssize_t read_fd(int fd, void *ptr, size_t nbytes, int *recvfd)
{
    struct msghdr msg;
    struct iovec iov[1];
    ssize_t n;

    union {
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    }control_un;

    struct cmsghdr *cmptr;
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);

    //-------------------------
    msg.msg_name = NULL;
    msg.msg_controllen = 0;

    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    if((n  = recvmsg(fd, &msg, MSG_WAITALL)) < 0)
    {
        return n;
    }

    if((cmptr = CMSG_FIRSTHDR(&msg)) != NULL && cmptr->cmsg_len == CMSG_LEN(sizeof(int)))
    {
        if(cmptr->cmsg_level != SOL_SOCKET)
        {
            err_msg("control level != SOL_SOCKET");
        }
        if(cmptr->cmsg_type != SCM_RIGHTS)
        {
            err_msg("control type != SCM_RIGHTS");
        }
        *recvfd = *((int *) CMSG_DATA(cmptr));
    }
    else
    {
        *recvfd = -1; // descroptor was not passed
    }

    return n;
}
