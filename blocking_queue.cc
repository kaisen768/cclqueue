#include "blocking_queue.h"
#include <stdlib.h>
#include <errno.h>

namespace cclqueue {

BlockingQueue::BlockingQueue(const uint32_t capacity)
{
    pthread_condattr_t cond_attr;

    capacity_ = ((capacity == 0) || (capacity > BLOCKING_QUEUE_MAX_CAPACITY)) ? \
                BLOCKING_QUEUE_MAX_CAPACITY : capacity;

    pthread_condattr_init(&cond_attr);
    pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
    pthread_mutex_init(&take_lock_, nullptr);
    pthread_cond_init(&not_empty_, &cond_attr);
    pthread_mutex_init(&put_lock_, nullptr);
    pthread_cond_init(&not_full_, &cond_attr);
    pthread_condattr_destroy(&cond_attr);

    head_ = new struct _Node_t;
    last_ = head_;
    count_.fetch_and(0x0);
}

BlockingQueue::~BlockingQueue()
{
    Free();
}

void BlockingQueue::EnQueue(struct _Node_t* node)
{
    last_->next = node;
    last_ = node;
}

void* BlockingQueue::DeQueue()
{
    struct _Node_t* h = head_;
    struct _Node_t* first = h->next;

    h->next = nullptr;
    head_ = first;

    void* x = (void*)first->item;
    first->item = nullptr;

    delete h;
    return x;
}

void BlockingQueue::FullyLock()
{
    pthread_mutex_lock(&take_lock_);
    pthread_mutex_lock(&put_lock_);
}

void BlockingQueue::FullyUnlock()
{
    pthread_mutex_unlock(&put_lock_);
    pthread_mutex_unlock(&take_lock_);
}

void BlockingQueue::SignalNotEmpty()
{
    pthread_mutex_lock(&take_lock_);
    pthread_cond_signal(&not_empty_);
    pthread_mutex_unlock(&take_lock_);
}

void BlockingQueue::SignalNotFull()
{
    pthread_mutex_lock(&put_lock_);
    pthread_cond_signal(&not_full_);
    pthread_mutex_unlock(&put_lock_);
}

uint32_t BlockingQueue::Size()
{
    return count_;
}

void BlockingQueue::Clear()
{
    int c;

    FullyLock();

    for (struct _Node_t* next, * current = head_->next; current != nullptr; current = next) {
        next = current->next;
        if (current->item != nullptr)
            free(const_cast<void*>(current->item));

        delete current;
    }

    head_->next = nullptr;
    last_ = head_;

    c = count_.fetch_and(0x0);
    if (c == capacity_)
        pthread_cond_signal(&not_full_);

    FullyUnlock();
}

void BlockingQueue::Free()
{
    Clear();

    pthread_cond_destroy(&not_empty_);
    pthread_cond_destroy(&not_full_);
    pthread_mutex_destroy(&take_lock_);
    pthread_mutex_destroy(&put_lock_);

    delete head_;
}

bool BlockingQueue::Offer(const void *element)
{
    uint32_t c;

    if (element == nullptr)
        return false;

    if (count_ == capacity_)
        return false;

    struct _Node_t *new_node = new struct _Node_t;
    if (new_node == nullptr) {
        errno = ENOMEM;
        return false;
    }

    new_node->item = element;

    pthread_mutex_lock(&put_lock_);
    if (count_ == capacity_)
        goto insert_full;

    EnQueue(new_node);

    c = count_.fetch_add(1);
    if ((c + 1) < capacity_)
        pthread_cond_signal(&not_full_);

    pthread_mutex_unlock(&put_lock_);

    if (c == 0)
        SignalNotEmpty();

    return true;

insert_full:
    pthread_mutex_unlock(&put_lock_);
    delete new_node;
    return false;
}

void* BlockingQueue::Poll()
{
    uint32_t c;
    void* item = nullptr;

    pthread_mutex_lock(&take_lock_);

    if (count_ == 0)
        goto take_empty;

    item = DeQueue();

    c = count_.fetch_sub(1);
    if (c > 1)
        pthread_cond_signal(&not_empty_);

    pthread_mutex_unlock(&take_lock_);
    if (c == capacity_)
        SignalNotFull();

    return item;

take_empty:
    pthread_mutex_unlock(&take_lock_);
    return nullptr;
}

void* BlockingQueue::Peek()
{
    return nullptr;
}

bool BlockingQueue::Put(const void* element)
{
    int c;

    struct _Node_t *new_node = (struct _Node_t *)malloc(sizeof(struct _Node_t));
    if (!new_node)
    {
        errno = ENOMEM;
        return false;
    }

    new_node->item = element;
    new_node->next = NULL;

    pthread_mutex_lock(&put_lock_);

    while (count_ == capacity_)
    {
        pthread_cond_wait(&not_full_, &put_lock_);
    }

    EnQueue(new_node);

    c = count_.fetch_add(1);
    if ((c + 1) < capacity_)
        pthread_cond_signal(&not_full_);

    pthread_mutex_unlock(&put_lock_);

    if (c == 0)
        SignalNotEmpty();

    return true;
}

void* BlockingQueue::Take()
{
    uint32_t c;
    void* item = nullptr;

    pthread_mutex_lock(&take_lock_);

    while (count_ == 0)
        pthread_cond_wait(&not_empty_, &take_lock_);

    item = DeQueue();

    c = count_.fetch_sub(1);
    if (c > 1)
        pthread_cond_signal(&not_empty_);

    pthread_mutex_unlock(&take_lock_);

    if (c == capacity_)
        SignalNotFull();

    return item;
}

}
