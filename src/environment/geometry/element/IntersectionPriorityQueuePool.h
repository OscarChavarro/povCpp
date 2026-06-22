#ifndef __INTERSECTION_PRIORITY_QUEUE_POOL__
#define __INTERSECTION_PRIORITY_QUEUE_POOL__

#include "java/util/PriorityQueue.h"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.h"

class IntersectionCandidate;

class IntersectionPriorityQueuePool {
  public:
    IntersectionPriorityQueuePool();
    ~IntersectionPriorityQueuePool();

    void init();
    java::PriorityQueue<IntersectionCandidate> *pop(int indexSize);
    void push(java::PriorityQueue<IntersectionCandidate> *queue);

  private:
    static constexpr int NUMBER_OF_PRIOQS = 32;
    static constexpr int MAX_NUMBER_OF_ENTRIES = 128;

    MemoryPool<java::PriorityQueue<IntersectionCandidate>> storage;
    java::PriorityQueue<IntersectionCandidate> *queues;
    int nextFreeIndex[NUMBER_OF_PRIOQS];
    int headIndex;
};

#endif
