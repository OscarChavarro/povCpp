#ifndef __INTERSECTION_PRIORITY_QUEUE_POOL__
#define __INTERSECTION_PRIORITY_QUEUE_POOL__

#include "java/util/PriorityQueue.h"

class Intersection;

class IntersectionPriorityQueuePool {
  public:
    IntersectionPriorityQueuePool();
    ~IntersectionPriorityQueuePool();

    void init();
    java::PriorityQueue<Intersection> *pop(int indexSize);
    void push(java::PriorityQueue<Intersection> *queue);

  private:
    static constexpr int NUMBER_OF_PRIOQS = 32;
    static constexpr int MAX_NUMBER_OF_ENTRIES = 128;

    java::PriorityQueue<Intersection> *queues;
    int nextFreeIndex[NUMBER_OF_PRIOQS];
    int headIndex;
};

#endif
