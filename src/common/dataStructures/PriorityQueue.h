#ifndef __PRIOQ_H__
#define __PRIOQ_H__

template <class T>
class PriorityQueue {
  private:
    void balance(unsigned int entryPos1);

  public:
    T *queue;
    unsigned int currentEntry;
    unsigned queueSize;
    PriorityQueue<T> *next_pq;

    PriorityQueue();
    void add(T *queueEntry);
    T *getHighest() const;
    void deleteHighest();
    void print() const;
};

#include "common/dataStructures/PriorityQueue.txx"

#endif
