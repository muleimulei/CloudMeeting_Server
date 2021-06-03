#include <iostream>
#include "unpthread.h"
#include "unp.h"
using namespace std;

Thread * tptr;
socklen_t addrlen;
int listenfd;

int main(int argc, char **argv)
{
    int i;
    void thread_make(int);

    if(argc == 3)
    {
        listenfd = Tcp_listen(NULL, argv[1], &addrlen);
    }
    else if(argc == 4)
    {
        listenfd = Tcp_listen(argv[1], argv[2], &addrlen);
    }
    else
    {
        err_quit("usage: ./app [host] <port #> <#threads>");
    }

    int nthreads = atoi(argv[argc - 1]);

    printf("total threads: %d\n", nthreads);

    tptr = (Thread *)Calloc(nthreads, sizeof(Thread));

    for(i = 0; i < nthreads; i++)
    {
        thread_make(i);
    }
    for(;;)
    {
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
//    printf(" %d\n", *arg);

    Pthread_create(&tptr[i].thread_tid, NULL, thread_main, arg);
}
