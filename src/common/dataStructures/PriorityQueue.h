#ifndef __PRIOQ_H__
#define __PRIOQ_H__

template <class T>
class PriorityQueue {
  private:
    void balance(unsigned int entryPos1);

  public:
    T *queue;
    unsigned int current_entry, queue_size;
    PriorityQueue<T> *next_pq;

    PriorityQueue();
    void add(T *queueEntry);
    T *getHighest();
    void deleteHighest();
};

#include "common/dataStructures/PriorityQueue.txx"

#endif
