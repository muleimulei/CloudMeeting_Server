#include "unpthread.h"
#include <stdlib.h>
#include "unp.h"
#include "netheader.h"

pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER; // accept lock
extern socklen_t addrlen;
extern int listenfd;
extern int nprocesses;
extern Room *room;

void* thread_main(void *arg)
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
    else if(ret < 12 || head[0] != '$')
    {
        printf("data len too short\n");
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

        uint32_t msgsize;
        memcpy(&msgsize, head + 7, 4);
        msgsize = ntohl(msgsize);

        if(msgtype == CREATE_MEETING)
        {
            if(head[11] == '#' && msgsize == 12)
            {
                char *c = (char *)&ip;
                printf("create meeting  ip: %d.%d.%d.%d\n", (unsigned char )c[3], (unsigned char )c[2], (uint)c[1], (uint)c[0]);
                if(room->navail <=0)
                {
                    write(connfd, "NoRoom", 7);
                }
                else
                {
                    int i;
                    //find room empty
                    Pthread_mutex_lock(&room->lock);

                    for(i = 0; i < nprocesses; i++)
                    {
                        if(room->pptr[i].child_status == 0) break;
                    }
                    if(i == nprocesses) //no room empty
                    {
                        write(connfd, "NoRoom", 7);
                    }
                    else
                    {
                        char cmd = 'C';
                        if(write_fd(room->pptr[i].child_pipefd, &cmd, 1, connfd) < 0)
                        {
                            printf("write fd error");
                        }
                        else
                        {
                            printf("room %d empty\n", room->pptr[i].child_pid);
                            room->pptr[i].child_status = 1; // occupy
                            room->navail--;
                        }
                    }
                    Pthread_mutex_unlock(&room->lock);
                }
                //write(connfd, "helloworld", 10);
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

