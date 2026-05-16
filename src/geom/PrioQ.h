#ifndef __PRIOQ_H__
#define __PRIOQ_H__

#include "geom/Intersection.h"
#include "geom/PriorityQueueNodeClass.h"

class PriorityQueue;

extern PriorityQueue *GLOBAL_priorityQueue;
extern PriorityQueueNode *GLOBAL_priorityQueuesHead;
extern PriorityQueueNode *pqInit(void);
extern PriorityQueueNode *pqPop(int index_size);

#endif
