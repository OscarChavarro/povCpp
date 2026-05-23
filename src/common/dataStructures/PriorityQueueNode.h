#ifndef __PRIORITY_QUEUE_NODE_H__
#define __PRIORITY_QUEUE_NODE_H__

#include "common/dataStructures/PriorityQueue.h"
#include "common/dataStructures/PriorityQueuePool.h"

class Intersection;

using PriorityQueueNode = PriorityQueue<Intersection>;
using IntersectionPriorityQueuePool = PriorityQueuePool<Intersection>;

#endif
