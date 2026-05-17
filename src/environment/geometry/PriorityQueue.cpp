/****************************************************************************
 *                     prioq.c
 *
 *  This module implements a priority queue using a heap.
 *
 *****************************************************************************/

#include "environment/geometry/PriorityQueue.h"
#include "common/FrameConfig.h"
#include "app/PovApp.h"

//===========================================================================

// used on pq_new and pq_init
static constexpr int NUMBER_OF_PRIOQS = 32;
static constexpr int MAX_NUMBER_OF_INTERSECTIONS = 128;

//===========================================================================

void
PriorityQueueNode::balance(unsigned int entryPos1)
{
    register Intersection *entry1;
    register Intersection *entry2;
    Intersection tempEntry;
    register unsigned int entryPos2;

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

/**
Called from all geometries
*/
void
PriorityQueueNode::add(Intersection *queueEntry)
{
    this->current_entry++;
    if (this->current_entry >= this->queue_size) {
        this->current_entry--;
    }
    this->queue[this->current_entry] = (*queueEntry);
    this->balance(this->current_entry);
}

/**
Called from objects.cpp, csg.cpp and lighting.cpp
*/
Intersection *
PriorityQueueNode::getHighest()
{
    if (this->current_entry >= 1) {
        return (&(this->queue[1]));
    }
    return nullptr;
}

/**
Called from objects.cpp, csg.cpp and lighting.cpp
*/
void
PriorityQueueNode::deleteHighest()
{
    this->queue[1] = this->queue[this->current_entry--];
    this->balance(1);
}

/**
Used from objects.cpp, csg.cpp and lighting.cpp
*/
void
PriorityQueueNode::pushBackToPool()
{
    this->next_pq = GLOBAL_priorityQueuesHead;
    GLOBAL_priorityQueuesHead = this;
}
