
#include "unpthread.h"
#include "msg.h"
#include "unp.h"
enum USER_TYPE
{
    GUEST=2,
    OWNER
};

typedef struct pool
{
    fd_set fdset;
    pthread_mutex_t lock;
    int owner;
    int num;
    int fds[1024 + 10];
    pool()
    {
        owner = 0;
        FD_ZERO(&fdset);
        lock = PTHREAD_MUTEX_INITIALIZER;
        memset(fds, 0, sizeof(fds));
        num = 0;
    }

    void clear_room()
    {
        Pthread_mutex_lock(&lock);
        owner = 0;
        FD_ZERO(&fdset);
        num = 0;
        memset(fds, 0, sizeof(fds));

    }
}Pool;

Pool * user_pool = new Pool();
static int maxfd;

void process_main(int i, int fd) // room start
{
    //create accpet fd thread
    printf("room %d starting \n", getpid());
    pthread_t pfd1;
    void* accept_fd(void *);
    int *ptr = (int *)malloc(4);
    *ptr = fd;
    Pthread_create(&pfd1, NULL, accept_fd, ptr); // accept fd


    //listen
    for(;;)
    {
        fd_set rset = user_pool->fdset;
        int nsel = Select(maxfd + 1, &rset, NULL, NULL, NULL);

        for(int i = 0; i < user_pool->num; i++)
        {
            //check data arrive
            if(FD_ISSET(user_pool->fds[i], &rset))
            {
                //read data

            }
            if(--nsel <= 0) break;
        }
    }
}


void* accept_fd(void *arg) //accept fd from father
{
    Pthread_detach(pthread_self());
    int fd = *(int *)arg, tfd = -1;
    free(arg);
    while(1)
    {
        int n, c;
        if((n = read_fd(fd, &c, 1, &tfd)) <= 0)
        {
            err_quit("read_fd error");
        }
        if(tfd < 0)
        {
            err_quit("no descriptor from read_fd");
        }

        //add to poll
        Pthread_mutex_lock(&user_pool->lock);
        if(c == 'C')
        {
            FD_SET(tfd, &user_pool->fdset);
            user_pool->owner = tfd;
            user_pool->fds[user_pool->num++] = tfd;
            maxfd = MAX(maxfd, tfd);
        }
        else if(c == 'J')
        {
            if(user_pool->num > 1024)
            {
                printf("room is too large\n");
                Pthread_mutex_unlock(&user_pool->lock);
                continue;
            }
            else
            {
                FD_SET(tfd, &user_pool->fdset);
                user_pool->fds[user_pool->num++] = tfd;
                maxfd = MAX(maxfd, tfd);
            }
        }
        Pthread_mutex_unlock(&user_pool->lock);
    }
}
