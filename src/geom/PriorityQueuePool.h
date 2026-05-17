#ifndef __PRIORITY_QUEUE_POOL_H__
#define __PRIORITY_QUEUE_POOL_H__

#include "geom/PriorityQueueNodeClass.h"

class PriorityQueuePool {
  public:
    static PriorityQueueNode *pqInit(void);
    static PriorityQueueNode *pqPop(int indexSize);
};

#endif
