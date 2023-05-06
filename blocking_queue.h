#ifndef BLOCKING_QUEUE_H_
#define BLOCKING_QUEUE_H_

#include <stdint.h>
#include <thread>
#include <atomic>

namespace cclqueue {

#define BLOCKING_QUEUE_MAX_CAPACITY 0xFFFFFFFFU

struct _Node_t
{
    const void* item;
    struct _Node_t* next;

    _Node_t() {
        item = nullptr;
        next = nullptr;
    }
};

class BlockingQueue
{
public:
    explicit BlockingQueue(const uint32_t capacity);
    ~BlockingQueue();

    /** 
     * @brief Returns the number of elements in this collection.
     *
     * @return the number of elements in this collection
     */
    uint32_t Size();

    /**
     * @brief Removes all of the elements from this collection.
     * 
     */
    void Clear();

    /**
     * @brief collection
     *
     */
    void Free();

    /** 
     * @brief Inserts the specified element into this queue if it is possible to do
     * so immediately without violating capacity restrictions.
     *
     * @param element element
     * @return true if the element was added to this queue, else false
     */
    bool Offer(const void* element);

    /** 
     * @brief Retrieves and removes the head of this queue,
     * or returns NULL if this queue is empty.
     *
     * @return the head of this queue, or NULL if this queue is empty 
     */
    void* Poll();

    /** 
     * @brief Retrieves, but does not remove, the head of this queue,
     * or returns NULL if this queue is empty.
     *
     * @return the head of this queue, or NULL if this queue is empty
     */
    void* Peek();

    /**
     * @brief Inserts the specified element into this queue, waiting if necessary for space to become available.
     *
     * @param element the element to add
     * @return true if the element was added to this queue, else false
     */
    bool Put(const void* element);

    /** 
     * @brief Retrieves and removes the head of this queue, waiting if necessary until an element becomes available.
     *
     * @return the head of this queue
     */
    void* Take();

private:
    void EnQueue(struct _Node_t* node);
    void* DeQueue();
    void FullyLock();
    void FullyUnlock();
    void SignalNotEmpty();
    void SignalNotFull();

private:
    uint32_t capacity_;
    std::atomic<int> count_;
    struct _Node_t* head_;
    struct _Node_t* last_;
    pthread_mutex_t take_lock_;
    pthread_cond_t not_empty_;
    pthread_mutex_t put_lock_;
    pthread_cond_t not_full_;
};

}

#endif /* BLOCKING_QUEUE_H_ */
