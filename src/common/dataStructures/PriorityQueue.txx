#ifndef __PRIOQ_TXX__
#define __PRIOQ_TXX__

#include <cstdio>
#include "vsdk/toolkit/common/logging/Logger.h"

template <class T>
PriorityQueue<T>::PriorityQueue()
{
    queue = nullptr;
    currentEntry = 0;
    queueSize = 0;
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

    if ((entryPos1 * 2 < this->queueSize) &&
        (entryPos1 * 2 <= this->currentEntry)) {
        if ((entryPos1 * 2 + 1 > this->currentEntry) ||
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
    this->currentEntry++;
    if (this->currentEntry >= this->queueSize) {
        this->currentEntry--;
    }
    this->queue[this->currentEntry] = (*queueEntry);
    this->balance(this->currentEntry);
}

template <class T>
T *
PriorityQueue<T>::getHighest() const
{
    if (this->currentEntry >= 1) {
        return (&(this->queue[1]));
    }
    return nullptr;
}

template <class T>
void
PriorityQueue<T>::deleteHighest()
{
    this->queue[1] = this->queue[this->currentEntry--];
    this->balance(1);
}

template <class T>
void
PriorityQueue<T>::print() const
{
    {
        char _logMsg[1024];
        snprintf(_logMsg, sizeof(_logMsg), "PriorityQueue size=%u entries=%u\n", this->queueSize, this->currentEntry);
        Logger::reportMessage("PriorityQueue", Logger::WARNING, "", _logMsg);
    }
}

#endif
