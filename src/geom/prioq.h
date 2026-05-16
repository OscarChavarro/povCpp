#ifndef __PRIOQ_H__
#define __PRIOQ_H__

#include "geom/Intersection.h"

class PriorityQueueNode
{
  private:
    void balance(unsigned int entry_pos1);

  public:
    Intersection *queue;
    unsigned int current_entry, queue_size;
    PriorityQueueNode *next_pq;

    void add(Intersection *queue_entry);
    Intersection * getHighest();
    void deleteHighest();
};

class PriorityQueue
{
  private:
    PriorityQueueNode *head;
  public:
    PriorityQueue();

    friend class PriorityQueueNode;
};

extern PriorityQueue *GLOBAL_priorityQueue;
extern PriorityQueueNode *GLOBAL_priorityQueuesHead;
extern PriorityQueueNode *pq_init(void);
extern PriorityQueueNode *pq_pop(int index_size);
extern void pq_push(PriorityQueueNode *pq);

#endif
