#ifndef __PRIORITY_QUEUE_POOL__
#define __PRIORITY_QUEUE_POOL__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"

template <class T>
class PriorityQueuePool {
  private:
    static constexpr int NUMBER_OF_PRIORITY_QUEUES = 32;
    static constexpr int MAX_NUMBER_OF_ENTRIES = 128;

    MemoryPool<java::PriorityQueue<T>> storage;
    java::PriorityQueue<T> *queues;
    int nextFreeIndex[NUMBER_OF_PRIORITY_QUEUES];
    int headIndex;

  public:
    PriorityQueuePool();
    ~PriorityQueuePool();

    void init();
    java::PriorityQueue<T> *pop(int indexSize);
    void push(java::PriorityQueue<T> *queue);
};

#endif
