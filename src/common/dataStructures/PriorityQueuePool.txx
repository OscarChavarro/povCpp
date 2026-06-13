#ifndef __PRIORITY_QUEUE_POOL_TXX__
#define __PRIORITY_QUEUE_POOL_TXX__

#include <cstdio>
#include <cstdlib>
#include "vsdk/toolkit/common/logging/Logger.h"

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
        newNode = new PriorityQueue<T>(MAX_NUMBER_OF_ENTRIES);
        if (newNode == nullptr) {
            Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of memory. Cannot allocate queues");
        }

        newNode->setPoolNext(head);
        head = newNode;
        newNode->resetForPool(0);
    }

    return head;
}

#endif
