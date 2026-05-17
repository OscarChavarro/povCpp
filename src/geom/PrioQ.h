#ifndef __PRIOQ_H__
#define __PRIOQ_H__

#include "geom/Intersection.h"
#include "geom/PriorityQueueNodeClass.h"

class PriorityQueue;

extern PriorityQueue *GLOBAL_priorityQueue;
extern PriorityQueueNode *GLOBAL_priorityQueuesHead;
class PriorityQueuePool {
  public:
    static PriorityQueueNode *pqInit(void);
    static PriorityQueueNode *pqPop(int indexSize);
};

#endif
