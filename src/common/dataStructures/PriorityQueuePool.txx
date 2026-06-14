#ifndef __PRIORITY_QUEUE_POOL_TXX__
#define __PRIORITY_QUEUE_POOL_TXX__

#include <cstdio>
#include <cstdlib>
#include <new>
#include "common/dataStructures/PriorityQueuePool.h"
#include "vsdk/toolkit/common/logging/Logger.h"

template <class T>
java::PriorityQueue<T> *PriorityQueuePool<T>::queues = nullptr;

template <class T>
int PriorityQueuePool<T>::nextFreeIndex[PriorityQueuePool<T>::NUMBER_OF_PRIOQS] = {};

template <class T>
int PriorityQueuePool<T>::headIndex = -1;

template <class T>
java::PriorityQueue<T> *
PriorityQueuePool<T>::pqInit()
{
    int i;

    if (queues == nullptr) {
        void * const rawStorage =
            ::operator new[](sizeof(java::PriorityQueue<T>) * NUMBER_OF_PRIOQS);
        queues = static_cast<java::PriorityQueue<T> *>(rawStorage);
        if (queues == nullptr) {
            Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of memory. Cannot allocate queue storage");
        }

        for (i = 0; i < NUMBER_OF_PRIOQS; i++) {
            new (queues + i) java::PriorityQueue<T>(MAX_NUMBER_OF_ENTRIES);
        }
    }

    for (i = 0; i < NUMBER_OF_PRIOQS; i++) {
        queues[i].clear();
        java::PriorityQueueAccess<T>::setActiveLimit(queues[i], -1);
        nextFreeIndex[i] = i + 1;
    }
    nextFreeIndex[NUMBER_OF_PRIOQS - 1] = -1;
    headIndex = 0;

    return queues;
}

#endif
