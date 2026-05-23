#ifndef __PRIORITY_QUEUE_POOL_H__
#define __PRIORITY_QUEUE_POOL_H__

#include "common/dataStructures/PriorityQueue.h"

template <class T>
class PriorityQueuePool {
  public:
    static PriorityQueue<T> *pqInit();
    static PriorityQueue<T> *pqPop(int indexSize);
    static void pqPush(PriorityQueue<T> *queue);

  private:
    static PriorityQueue<T> *head;
};

#include "common/dataStructures/PriorityQueuePool.txx"

#endif
