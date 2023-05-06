#include "blocking_queue.h"
#include <unistd.h>
#include <iostream>
#include <thread>

static pthread_cond_t sendsig;
static pthread_mutex_t handlemutex;

void recvhandler(void* arg)
{
    int* err;
    int last;
    bool dolast = false;
    cclqueue::BlockingQueue* queue = static_cast<cclqueue::BlockingQueue*>(arg);

    while (1)
    {
        pthread_mutex_lock(&handlemutex);
        pthread_cond_wait(&sendsig, &handlemutex);

        std::cout << "action start" << std::endl;

        do
        {
            err = static_cast<int*>(queue->Poll());

            if (err) {
                if (dolast) {
                    if ((*err - last) > 1)
                        std::cout << "current : " << *err << ", last : " << last << std::endl;
                }

                last = *err;
                dolast = true;

                std::cout << "recvhandler element : " << *err << std::endl;
                free(err);
            }
        } while (err);

        pthread_mutex_unlock(&handlemutex);
    }
}

int main(int argc, char const *argv[])
{
    cclqueue::BlockingQueue queue(0);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    pthread_cond_init(&sendsig, &cond_attr);
    pthread_mutex_init(&handlemutex, nullptr);
    pthread_condattr_destroy(&cond_attr);

    std::thread recvthread(recvhandler, &queue);
    recvthread.detach();

    sleep(2);

    int i = 0;
    while (i < 10000) {
        int* num = static_cast<int*>(malloc(sizeof(int)));
        *num = i;

        queue.Offer(num);
        pthread_cond_signal(&sendsig);

        ++i;
    }

    pause();

    return 0;
}
