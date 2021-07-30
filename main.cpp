#include <iostream>
#include "unpthread.h"
#include "unp.h"
using namespace std;

Thread * tptr;
socklen_t addrlen;
int listenfd;
int navail, nprocesses;

Room *room;
//typedef struct Room_Pool
//{
//    Room *room_pool[100]; //pool
//    int num;

//    Room_Pool()
//    {
//        memset(room_pool, 0, sizeof(room_pool));
//        num = 0;
//    }
//}Room_Pool;

int main(int argc, char **argv)
{
    void sig_chld(int signo);
    Signal(SIGCHLD, sig_chld);
    int i,maxfd;
    void thread_make(int);
    void process_make(int, int);

    fd_set rset, masterset;
    FD_ZERO(&masterset);

    if(argc == 4)
    {
        listenfd = Tcp_listen(NULL, argv[1], &addrlen);
    }
    else if(argc == 5)
    {
        listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
    }
    else
    {
        err_quit("usage: ./app [host] <port #> <#threads> <#processes>");
    }
    maxfd = listenfd;
    int nthreads = atoi(argv[argc - 2]);
    nprocesses = atoi(argv[argc-1]);

    //init room
    room = new Room(nprocesses);

    printf("total threads: %d  total process: %d\n", nthreads, nprocesses);

    tptr = (Thread *)Calloc(nthreads, sizeof(Thread));

    //process pool----room
    for(i = 0; i < nprocesses; i++)
    {
        process_make(i, listenfd);
        FD_SET(room->pptr[i].child_pipefd, &masterset);
        maxfd = max(maxfd, room->pptr[i].child_pipefd);
    }

    //thread pool
    for(i = 0; i < nthreads; i++)
    {
        thread_make(i);
    }
    for(;;)
    {
        //listen
        rset = masterset;

        int nsel = Select(maxfd + 1, &rset, NULL, NULL, NULL);
        if(nsel == 0) continue;

        //set room status to 0(empty)
        for(i = 0; i < nprocesses; i++)
        {
            if(FD_ISSET(room->pptr[i].child_pipefd, &rset))
            {
                char rc;
				int  n;
                if((n = Readn(room->pptr[i].child_pipefd, &rc, 1)) <= 0)
                {
                    err_quit("child %d terminated unexpectedly", i);
                }
				printf("c = %c\n", rc);
                if(rc == 'E') // room empty
                {
                    pthread_mutex_lock(&room->lock);
                    room->pptr[i].child_status = 0;
                    room->navail++;
                    printf("room %d is now free\n", room->pptr[i].child_pid);
                    pthread_mutex_unlock(&room->lock);

                }
                else if(rc == 'Q') // partner quit
                {
                    Pthread_mutex_lock(&room->lock);
                    room->pptr[i].total--;
                    Pthread_mutex_unlock(&room->lock);
                }
                else // trash data
                {
                    err_msg("read from %d error", room->pptr[i].child_pipefd);
                    continue;
                }
                if(--nsel == 0) break; /*all done with select results*/
            }

        }
    }
    return 0;
}


// create threads
void thread_make(int i)
{
    void * thread_main(void *);
    int *arg = (int *) Calloc(1, sizeof(int));
    *arg = i;
    Pthread_create(&tptr[i].thread_tid, NULL, thread_main, arg);
}


int process_make(int i, int listenfd)
{
    int sockfd[2];
    pid_t pid;
    void process_main(int, int);

    Socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
    if((pid = fork()) > 0)
    {
        Close(sockfd[1]);
        room->pptr[i].child_pid = pid;
        room->pptr[i].child_pipefd = sockfd[0];
        room->pptr[i].child_status = 0;
        room->pptr[i].total = 0;
        return pid; // father
    }

    Close(listenfd); // child not need this open
    Close(sockfd[0]);
    process_main(i, sockfd[1]); /* never returns */
}
