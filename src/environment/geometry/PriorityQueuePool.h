#ifndef __PRIORITY_QUEUE_POOL_H__
#define __PRIORITY_QUEUE_POOL_H__

#include "environment/geometry/PriorityQueueNode.h"

class PriorityQueuePool {
  public:
    static PriorityQueueNode *pqInit(void);
    static PriorityQueueNode *pqPop(int indexSize);
};

#endif
