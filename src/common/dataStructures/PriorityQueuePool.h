#ifndef __PRIORITY_QUEUE_POOL_H__
#define __PRIORITY_QUEUE_POOL_H__

#include "common/dataStructures/PriorityQueue.h"
#include "vsdk/toolkit/common/logging/Logger.h"

template <class T>
class PriorityQueuePool {
  public:
    static PriorityQueue<T> *pqInit();

    static inline PriorityQueue<T> *pqPop(int indexSize) {
        static constexpr int MAX_NUMBER_OF_ENTRIES = 128;
        if (indexSize >= MAX_NUMBER_OF_ENTRIES) indexSize = MAX_NUMBER_OF_ENTRIES - 1;
        if (head == nullptr) {
            Logger::reportMessage("PriorityQueuePool", Logger::FATAL_ERROR, "", "\nOut of prioqs");
        }
        PriorityQueue<T> *pq = head;
        if (pq == nullptr) return nullptr;
        head = pq->next_pq;
        pq->queueSize = indexSize;
        pq->currentEntry = 0;
        return pq;
    }

    static inline void pqPush(PriorityQueue<T> *queue) {
        queue->next_pq = head;
        head = queue;
    }

  private:
    static PriorityQueue<T> *head;
};

#include "common/dataStructures/PriorityQueuePool.txx"

#endif
