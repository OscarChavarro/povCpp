#ifndef __PRIOQ_TXX__
#define __PRIOQ_TXX__

#include "common/logger/Logger.h"

template <class T>
PriorityQueue<T>::PriorityQueue()
{
    queue = nullptr;
    current_entry = 0;
    queue_size = 0;
    next_pq = nullptr;
}

template <class T>
void
PriorityQueue<T>::balance(unsigned int entryPos1)
{
    T *entry1;
    T *entry2;
    T tempEntry;
    unsigned int entryPos2;

    entry1 = &this->queue[entryPos1];

    if ((entryPos1 * 2 < this->queue_size) &&
        (entryPos1 * 2 <= this->current_entry)) {
        if ((entryPos1 * 2 + 1 > this->current_entry) ||
            (this->queue[entryPos1 * 2].Depth <
                this->queue[entryPos1 * 2 + 1].Depth)) {
            entryPos2 = entryPos1 * 2;
        } else {
            entryPos2 = entryPos1 * 2 + 1;
        }

        entry2 = &this->queue[entryPos2];

        if (entry1->Depth > entry2->Depth) {
            tempEntry = *entry1;
            *entry1 = *entry2;
            *entry2 = tempEntry;
            this->balance(entryPos2);
        }
    }

    if (entryPos1 / 2 >= 1) {
        entryPos2 = entryPos1 / 2;
        entry2 = &this->queue[entryPos2];
        if (entry1->Depth < entry2->Depth) {
            tempEntry = *entry1;
            *entry1 = *entry2;
            *entry2 = tempEntry;
            this->balance(entryPos2);
        }
    }
}

template <class T>
void
PriorityQueue<T>::add(T *queueEntry)
{
    this->current_entry++;
    if (this->current_entry >= this->queue_size) {
        this->current_entry--;
    }
    this->queue[this->current_entry] = (*queueEntry);
    this->balance(this->current_entry);
}

template <class T>
T *
PriorityQueue<T>::getHighest()
{
    if (this->current_entry >= 1) {
        return (&(this->queue[1]));
    }
    return nullptr;
}

template <class T>
void
PriorityQueue<T>::deleteHighest()
{
    this->queue[1] = this->queue[this->current_entry--];
    this->balance(1);
}

template <class T>
void
PriorityQueue<T>::print()
{
    Logger::info("PriorityQueue size=%u entries=%u\n", this->queue_size,
        this->current_entry);
}

#endif
