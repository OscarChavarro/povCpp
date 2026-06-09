#ifndef __PRIORITY_QUEUE_POOL_TXX__
#define __PRIORITY_QUEUE_POOL_TXX__

#include "vsdk/toolkit/common/logging/Logger.h"

#include <cstdio>
#include <cstdlib>

template <class T>
PriorityQueue<T> *PriorityQueuePool<T>::head = nullptr;

template <class T>
PriorityQueue<T> *
PriorityQueuePool<T>::pqInit()
{
    static constexpr int NUMBER_OF_PRIOQS = 32;
    static constexpr int MAX_NUMBER_OF_ENTRIES = 128;

    int i;
    PriorityQueue<T> *newNode;

    head = nullptr;

    for (i = 0; i < NUMBER_OF_PRIOQS; i++) {
        newNode = new PriorityQueue<T>();
        if (newNode == nullptr) {
            Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of memory. Cannot allocate queues");
        }

        newNode->next_pq = head;
        head = newNode;

        newNode->queue = new T[MAX_NUMBER_OF_ENTRIES];
        if (newNode->queue == nullptr) {
            Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of memory. Cannot allocate queue entries");
        }
        newNode->currentEntry = 0;
        newNode->queueSize = 0;
    }

    return head;
}

template <class T>
PriorityQueue<T> *
PriorityQueuePool<T>::pqPop(int indexSize)
{
    static constexpr int MAX_NUMBER_OF_ENTRIES = 128;

    if (indexSize >= MAX_NUMBER_OF_ENTRIES) {
        indexSize = MAX_NUMBER_OF_ENTRIES - 1;
    }

    PriorityQueue<T> *pq;

    if (head == nullptr) {
        Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of prioqs");
    }
    pq = head;

    if (pq == nullptr) {
        return nullptr;
    }

    head = pq->next_pq;
    pq->queueSize = indexSize;
    pq->currentEntry = 0;
    return pq;
}

template <class T>
void
PriorityQueuePool<T>::pqPush(PriorityQueue<T> *queue)
{
    queue->next_pq = head;
    head = queue;
}

#endif
