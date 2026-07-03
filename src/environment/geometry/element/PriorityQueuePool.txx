#ifndef __PRIORITY_QUEUE_POOL_TXX__
#define __PRIORITY_QUEUE_POOL_TXX__

#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/geometry/element/PriorityQueuePool.h"

template <class T>
PriorityQueuePool<T>::PriorityQueuePool()
    : queues(nullptr), nextFreeIndex{}, headIndex(-1)
{
}

template <class T>
PriorityQueuePool<T>::~PriorityQueuePool()
{
    if (queues == nullptr) {
        return;
    }

    using QueueType = java::PriorityQueue<T>;
    for (int i = 0; i < NUMBER_OF_PRIORITY_QUEUES; i++) {
        queues[i].~QueueType();
    }
}

template <class T>
void
PriorityQueuePool<T>::init()
{
    if (queues == nullptr) {
        storage.init(sizeof(java::PriorityQueue<T>) * NUMBER_OF_PRIORITY_QUEUES);
        queues = storage.allocate(NUMBER_OF_PRIORITY_QUEUES);
        if (queues == nullptr) {
            Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of memory. Cannot allocate queue storage");
        }

        for (int i = 0; i < NUMBER_OF_PRIORITY_QUEUES; i++) {
            new (queues + i) java::PriorityQueue<T>(MAX_NUMBER_OF_ENTRIES);
        }
    }

    for (int i = 0; i < NUMBER_OF_PRIORITY_QUEUES; i++) {
        queues[i].clear();
        java::PriorityQueueAccess<T>::setActiveLimit(queues[i], -1);
        nextFreeIndex[i] = i + 1;
    }
    nextFreeIndex[NUMBER_OF_PRIORITY_QUEUES - 1] = -1;
    headIndex = 0;
}

template <class T>
inline java::PriorityQueue<T> *
PriorityQueuePool<T>::pop(int indexSize)
{
    if (indexSize >= MAX_NUMBER_OF_ENTRIES) {
        indexSize = MAX_NUMBER_OF_ENTRIES - 1;
    }
    if (queues == nullptr) {
        init();
    }
    if (headIndex < 0) {
        Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of prioqs");
    }

    const int index = headIndex;
    java::PriorityQueue<T> * const pq = queues + index;
    headIndex = nextFreeIndex[index];
    pq->clear();
    java::PriorityQueueAccess<T>::setActiveLimit(*pq, indexSize);
    return pq;
}

template <class T>
inline void
PriorityQueuePool<T>::push(java::PriorityQueue<T> *queue)
{
    if (queues == nullptr || queue < queues || queue >= queues + NUMBER_OF_PRIORITY_QUEUES) {
        Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nInvalid prioq");
    }

    const int index = static_cast<int>(queue - queues);
    queue->clear();
    java::PriorityQueueAccess<T>::setActiveLimit(*queue, -1);
    nextFreeIndex[index] = headIndex;
    headIndex = index;
}

#endif
