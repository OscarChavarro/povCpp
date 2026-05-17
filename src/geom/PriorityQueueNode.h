#ifndef __PRIORITY_QUEUE_NODE_H__
#define __PRIORITY_QUEUE_NODE_H__

#include "geom/Intersection.h"

class PriorityQueueNode {
  private:
    void balance(unsigned int entry_pos1);

  public:
    Intersection *queue;
    unsigned int current_entry, queue_size;
    PriorityQueueNode *next_pq;

    void add(Intersection *queue_entry);
    Intersection *getHighest();
    void deleteHighest();
    void pushBackToPool();
};

#endif
