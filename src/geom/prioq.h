#ifndef __PRIOQ_H__
#define __PRIOQ_H__

#include "geom/Intersection.h"
#include "geom/PriorityQueueNodeClass.h"

class PriorityQueue;

extern PriorityQueue *GLOBAL_priorityQueue;
extern PriorityQueueNode *GLOBAL_priorityQueuesHead;
extern PriorityQueueNode *pq_init(void);
extern PriorityQueueNode *pq_pop(int index_size);

#endif
