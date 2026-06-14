#ifndef __PRIORITY_QUEUE_POOL_H__
#define __PRIORITY_QUEUE_POOL_H__

#include "vsdk/toolkit/common/logging/Logger.h"
#include "java/util/PriorityQueue.h"

template <class T>
class PriorityQueuePool {
  public:
    static java::PriorityQueue<T> *pqInit();

    static inline java::PriorityQueue<T> *pqPop(int indexSize) {
        static constexpr int MAX_NUMBER_OF_ENTRIES = 128;
        if (indexSize >= MAX_NUMBER_OF_ENTRIES) indexSize = MAX_NUMBER_OF_ENTRIES - 1;
        if (queues == nullptr) {
            pqInit();
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

    static inline void pqPush(java::PriorityQueue<T> *queue) {
        if (queues == nullptr || queue < queues || queue >= queues + NUMBER_OF_PRIOQS) {
            Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nInvalid prioq");
        }
        const int index = static_cast<int>(queue - queues);
        queue->clear();
        java::PriorityQueueAccess<T>::setActiveLimit(*queue, -1);
        nextFreeIndex[index] = headIndex;
        headIndex = index;
    }

  private:
    static constexpr int NUMBER_OF_PRIOQS = 32;
    static constexpr int MAX_NUMBER_OF_ENTRIES = 128;

    static java::PriorityQueue<T> *queues;
    static int nextFreeIndex[NUMBER_OF_PRIOQS];
    static int headIndex;
};

#include "common/dataStructures/PriorityQueuePool.txx"

#endif
