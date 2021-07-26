#ifndef MSG_H
#define MSG_H

#include <queue>
#include "unp.h"
#include "unpthread.h"
#include "netheader.h"
#define MAXSIZE 10000
#define MB (1024*1024)


enum STATUS
{

    CLOSE = 0,
    ON = 1,
};

struct MSG
{
    char *ptr;
    int len;
    int targetfd;
    MSG_TYPE msgType;
    uint32_t ip;
    Image_Format format;

    MSG()
    {

    }
    MSG(MSG_TYPE msg_type, char *msg, int length, int fd)
    {
        msgType = msg_type;
        ptr = msg;
        len = length;
        targetfd = fd;
    }
};

struct SEND_QUEUE
{
private:
    pthread_mutex_t lock;
    pthread_cond_t cond;
    std::queue<MSG> send_queue;

public:
    SEND_QUEUE()
    {
        lock = PTHREAD_MUTEX_INITIALIZER;
        cond = PTHREAD_COND_INITIALIZER;
    }

    void push_msg(MSG msg)
    {
        Pthread_mutex_lock(&lock);
        while(send_queue.size() >= MAXSIZE)
        {
            Pthread_cond_wait(&cond, &lock);
        }
        send_queue.push(msg);
        Pthread_mutex_unlock(&lock);
        Pthread_cond_signal(&cond);
    }

    MSG pop_msg()
    {
        Pthread_mutex_lock(&lock);
        while(send_queue.empty())
        {
            Pthread_cond_wait(&cond, &lock);
        }
        MSG msg = send_queue.front();
        send_queue.pop();
        Pthread_mutex_unlock(&lock);
        Pthread_cond_signal(&cond);
        return msg;
    }
    void clear()
    {
        Pthread_mutex_lock(&lock);
        while(!send_queue.empty())
        {
            send_queue.pop();
        }
        Pthread_mutex_unlock(&lock);
    }
};

#endif // MSG_H
