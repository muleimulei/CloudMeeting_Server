#include "unpthread.h"
#include <stdlib.h>
#include <unp.h>
#include "netheader.h"

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER; // accept lock
extern socklen_t addrlen;
extern int listenfd;

void * thread_main(void *arg)
{
    void dowithuser(int connfd);
    int i = *(int *)arg;
    free(arg); //free
    int connfd;
    Pthread_detach(pthread_self()); //detach child thread

    printf("thread %d starting\n", i);

    SA *cliaddr;
    socklen_t clilen;
    cliaddr = (SA *)Calloc(1, addrlen);
    char buf[MAXSOCKADDR];
    for(;;)
    {
        clilen = addrlen;
        //lock accept
        Pthread_mutex_lock(&mlock);
        connfd = Accept(listenfd, cliaddr, &clilen);
        //unlock accept
        Pthread_mutex_unlock(&mlock);

        printf("connection from %s\n", Sock_ntop(buf, MAXSOCKADDR, cliaddr, clilen));

        dowithuser(connfd); // process user

        close(connfd); //close
    }
    return NULL;
}

void dowithuser(int connfd)
{
    char head[15]  = {0};
    ssize_t ret = Readn(connfd, head, 12);
    printf("%d\n", ret);
    if(ret <= 0)
    {
        printf("peer close\n");
    }
    else if(ret < 12)
    {
        printf("data format error\n");
    }
    else
    {
        //solve
        uint32_t ip;
        memcpy(&ip, head + 1, 4);
        ip = ntohl(ip);



        MSG_TYPE msgtype;
        memcpy(&msgtype, head + 5, 2);
        msgtype = (MSG_TYPE)ntohs(msgtype);

        printf("type: %d\n", msgtype);

        uint32_t msgsize;
        memcpy(&msgsize, head + 7, 4);
        msgsize = ntohl(msgsize);
        printf("size: %d\n", msgsize);


        if(msgtype == CREATE_MEETING)
        {
            if(head[11] == '#' && msgsize == 12)
            {
                printf("create meeting\n");
            }
            else
            {
                printf("data format error\n");
            }
        }
        else
        {
            printf("data format error\n");
        }
    }

}

