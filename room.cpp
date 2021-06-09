#include "unpthread.h"
#include "msg.h"
#include "unp.h"
#define SENDTHREADSIZE 5
SEND_QUEUE sendqueue; //save data

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
    void* send_func(void *);
    int *ptr = (int *)malloc(4);
    *ptr = fd;
    Pthread_create(&pfd1, NULL, accept_fd, ptr); // accept fd
    for(int i = 0; i < SENDTHREADSIZE; i++)
    {
        Pthread_create(&pfd1, NULL, send_func, NULL);
    }


    //listen read data from fds
    for(;;)
    {
        fd_set rset;
        int nsel;
        struct timeval time;
        memset(&time, 0, sizeof(struct timeval));
        while((nsel =  Select(maxfd + 1, &rset, NULL, NULL, &time))== 0)
        {
            rset = user_pool->fdset;
        }

        for(int i = 0; i < user_pool->num; i++)
        {
            //check data arrive
            if(FD_ISSET(user_pool->fds[i], &rset))
            {
                //read data
                printf("%d data arrive\n", user_pool->fds[i]); // not finished
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
            printf("c = %c\n", c);
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

            //write room No to  tfd


            MSG msg;
            msg.msgType = (MSG_TYPE)htonl( CREATE_MEETING_RESPONSE );
            msg.targetfd = tfd;
            int roomNo = htonl(getpid());
            msg.ptr = (char *) malloc(sizeof(int));
            memcpy(msg.ptr, &roomNo, sizeof(int));
            msg.len = sizeof(int);
            sendqueue.push_msg(msg);

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
    return NULL;
}

void *send_func(void *arg)
{
    Pthread_detach(pthread_self());
    char * sendbuf = (char *)malloc(4 * MB);
    /*
     * $_msgType_ip_size_data_#
    */

    for(;;)
    {
        memset(sendbuf, 0, 4 * MB);
        MSG msg = sendqueue.pop_msg();
        int len = 0;

        sendbuf[len++] = '$';

        if(msg.msgType == CREATE_MEETING_RESPONSE)
        {

            memcpy(sendbuf + len, &msg.msgType, sizeof(short)); //msgtype
            len += 6;
            int msglen = htonl(msg.len);
            memcpy(sendbuf + len, &msglen, sizeof(int));
            len+=4;
            memcpy(sendbuf + len, msg.ptr, msg.len);
            len+=msg.len;
            sendbuf[len++] = '#';
            //send buf to target
            if(writen(msg.targetfd, sendbuf, len) < 0)
            {
                err_msg("writen error");
            }
        }

        //free
        if(msg.ptr)
        {
            free(msg.ptr);
        }
    }

    return NULL;
}
