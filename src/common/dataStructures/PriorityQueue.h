#ifndef __PRIOQ_H__
#define __PRIOQ_H__

#include "environment/geometry/Intersection.h"
#include "common/dataStructures/PriorityQueueNode.h"
#include "common/dataStructures/PriorityQueuePool.h"

class PriorityQueue;

extern PriorityQueue *GLOBAL_priorityQueue;
extern PriorityQueueNode *GLOBAL_priorityQueuesHead;

#endif
