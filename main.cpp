#include <iostream>
#include "unpthread.h"
#include "unp.h"
using namespace std;

Thread * tptr;
Process *pptr;
socklen_t addrlen;
int listenfd;

int main(int argc, char **argv)
{
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
    int nprocesses = atoi(argv[argc-1]);

    printf("total threads: %d\n", nthreads);

    tptr = (Thread *)Calloc(nthreads, sizeof(Thread));
    pptr = (Process *)Calloc(nprocesses, sizeof(Process));


    for(i = 0; i < nprocesses; i++)
    {
        process_make(i, listenfd);
        FD_SET(pptr[i].child_pipefd, &masterset);
        maxfd = max(maxfd, pptr[i].child_pipefd);
    }

    for(i = 0; i < nthreads; i++)
    {
        thread_make(i);
    }
    for(;;)
    {
        //listen
        pause();
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


void process_make(int i, int listenfd)
{
    int sockfd[2];
    pid_t pid;
    void process_main(int, int);

    Socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd);
    if((pid = fork()) > 0)
    {
        Close(sockfd[1]);

        pptr[i].child_pid = pid;
        pptr[i].child_pipefd = sockfd[0];
        pptr[i].child_status = 0;

        return pid; // father
    }

    Close(listenfd); // child not need this open
    Close(sockfd[0]);
    process_main(i, sockfd[1]); /* never returns */
}
