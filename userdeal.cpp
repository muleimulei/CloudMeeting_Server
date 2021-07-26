#include "unpthread.h"
#include <stdlib.h>
#include "unp.h"
#include "netheader.h"
#include "msg.h"

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

//    printf("thread %d starting\n", i);

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


    }
    return NULL;
}


/*
 *
 *read data from client
 *
 */

void dowithuser(int connfd)
{
    void writetofd(int fd, MSG msg);

    char head[15]  = {0};
    //read head
    while(1)
    {
        ssize_t ret = Readn(connfd, head, 11);
        if(ret <= 0)
        {
            close(connfd); //close
            printf("%d close\n", connfd);
            return;
        }
        else if(ret < 11)
        {
            printf("data len too short\n");
        }
        else if(head[0] != '$')
        {
            printf("data format error\n");
        }
        else
        {
            //solve datatype
            MSG_TYPE msgtype;
            memcpy(&msgtype, head + 1, 2);
            msgtype = (MSG_TYPE)ntohs(msgtype);

            //solve ip
            uint32_t ip;
            memcpy(&ip, head + 3, 4);
            ip = ntohl(ip);

            //solve datasize
            uint32_t datasize;
            memcpy(&datasize, head + 7, 4);
            datasize = ntohl(datasize);

    //        printf("msg type %d\n", msgtype);

            if(msgtype == CREATE_MEETING)
            {
                char tail;
                Readn(connfd, &tail, 1);
                //read data from client
                if(datasize == 0 && tail == '#')
                {
                    char *c = (char *)&ip;
                    printf("create meeting  ip: %d.%d.%d.%d\n", (u_char)c[3], (u_char)c[2], (u_char)c[1], (u_char)c[0]);
                    if(room->navail <=0) // no room
                    {
                        MSG msg;
                        memset(&msg, 0, sizeof(msg));
                        msg.msgType = CREATE_MEETING_RESPONSE;
                        int roomNo = 0;
                        msg.ptr = (char *) malloc(sizeof(int));
                        memcpy(msg.ptr, &roomNo, sizeof(int));
                        msg.len = sizeof(roomNo);
                        writetofd(connfd, msg);
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
                            MSG msg;
                            memset(&msg, 0, sizeof(msg));
                            msg.msgType = CREATE_MEETING_RESPONSE;
                            int roomNo = 0;
                            msg.ptr = (char *) malloc(sizeof(int));
                            memcpy(msg.ptr, &roomNo, sizeof(int));
                            msg.len = sizeof(roomNo);
                            writetofd(connfd, msg);
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
                                close(connfd);
                                printf("room %d empty\n", room->pptr[i].child_pid);
                                room->pptr[i].child_status = 1; // occupy
                                room->navail--;
                                room->pptr[i].total++;
                                Pthread_mutex_unlock(&room->lock);
                                return;
                            }
                        }
                        Pthread_mutex_unlock(&room->lock);

                    }
                }
                else
                {
                    printf("1 data format error\n");
                }
            }
            else if(msgtype == JOIN_MEETING)
            {
                //read msgsize
                uint32_t msgsize, roomno;
                memcpy(&msgsize, head + 7, 4);
                msgsize = ntohl(msgsize);
                //read data + #
                int r =  Readn(connfd, head, msgsize + 1 );
                if(r < msgsize + 1)
                {
                    printf("data too short\n");
                }
                else
                {
                    if(head[msgsize] == '#')
                    {
                        memcpy(&roomno, head, msgsize);
                        roomno = ntohl(roomno);
    //                    printf("room : %d\n", roomno);
                        //find room no
                        bool ok = false;
                        int i;
                        for(i = 0; i < nprocesses; i++)
                        {
                            if(room->pptr[i].child_pid == roomno && room->pptr[i].child_status == 1)
                            {
                                ok = true; //find room
                                break;
                            }
                        }

                        MSG msg;
                        memset(&msg, 0, sizeof(msg));
                        msg.msgType = JOIN_MEETING_RESPONSE;
                        msg.len = sizeof(uint32_t);
                        if(ok)
                        {
                            if(room->pptr[i].total >= 1024)
                            {
                                msg.ptr = (char *)malloc(msg.len);
                                uint32_t full = -1;
                                memcpy(msg.ptr, &full, sizeof(uint32_t));
                                writetofd(connfd, msg);
                            }
                            else
                            {
                                Pthread_mutex_lock(&room->lock);

                                char cmd = 'J';
    //                            printf("i  =  %d\n", i);
                                if(write_fd(room->pptr[i].child_pipefd, &cmd, 1, connfd) < 0)
                                {
                                    err_msg("write fd:");
                                }
                                else
                                {

                                    msg.ptr = (char *)malloc(msg.len);
                                    memcpy(msg.ptr, &roomno, sizeof(uint32_t));
                                    writetofd(connfd, msg);
                                    room->pptr[i].total++;// add 1
                                    Pthread_mutex_unlock(&room->lock);
                                    close(connfd);
                                    return;
                                }
                                Pthread_mutex_unlock(&room->lock);
                            }
                        }
                        else
                        {
                            msg.ptr = (char *)malloc(msg.len);
                            uint32_t fail = 0;
                            memcpy(msg.ptr, &fail, sizeof(uint32_t));
                            writetofd(connfd, msg);
                        }
                    }
                    else
                    {
                        printf("format error\n");
                    }
                }
            }
            else
            {
                printf("data format error\n");
            }
        }
    }
}


void writetofd(int fd, MSG msg)
{
    char *buf = (char *) malloc(100);
    memset(buf, 0, 100);
    int bytestowrite = 0;
    buf[bytestowrite++] = '$';

    uint16_t type = msg.msgType;
    type = htons(type);
    memcpy(buf + bytestowrite, &type, sizeof(uint16_t));

    bytestowrite += 2;//skip type

    bytestowrite += 4; //skip ip

    uint32_t size = msg.len;
    size = htonl(size);
    memcpy(buf + bytestowrite, &size, sizeof(uint32_t));
    bytestowrite += 4;

    memcpy(buf + bytestowrite, msg.ptr, msg.len);
    bytestowrite += msg.len;

    buf[bytestowrite++] = '#';

    if(writen(fd, buf, bytestowrite) < bytestowrite)
    {
        printf("write fail\n");
    }

    if(msg.ptr)
    {
        free(msg.ptr);
        msg.ptr = NULL;
    }
    free(buf);
}
