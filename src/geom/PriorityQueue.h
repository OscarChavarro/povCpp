#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__

#include "geom/PriorityQueueNodeClass.h"

class PriorityQueue {
  private:
    PriorityQueueNode *head;

  public:
    PriorityQueue();

    friend class PriorityQueueNode;
};

#endif
