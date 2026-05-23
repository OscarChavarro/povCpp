#ifndef __PRIORITY_QUEUE_POOL_TXX__
#define __PRIORITY_QUEUE_POOL_TXX__

#include "app/PovApp.h"
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
            Logger::error( "\nOut of memory. Cannot allocate queues");
            PovApp::closeAll();
            exit(1);
        }

        newNode->next_pq = head;
        head = newNode;

        newNode->queue = new T[MAX_NUMBER_OF_ENTRIES];
        if (newNode->queue == nullptr) {
            Logger::error( "\nOut of memory. Cannot allocate queue entries");
            PovApp::closeAll();
            exit(1);
        }
        newNode->current_entry = 0;
        newNode->queue_size = 0;
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
        Logger::error( "\nOut of prioqs");
        PovApp::closeAll();
        exit(1);
    }
    pq = head;

    if (pq == nullptr) {
        return nullptr;
    }

    head = pq->next_pq;
    pq->queue_size = indexSize;
    pq->current_entry = 0;
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
