/****************************************************************************
 *                     prioq.c
 *
 *  This module implements a priority queue using a heap.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "geom/PrioQ.h"
#include "common/Frame.h"
#include "common/PovProto.h"

//===========================================================================

// used on pq_new and pq_init
#define NUMBER_OF_PRIOQS 32
#define MAX_NUMBER_OF_INTERSECTIONS 128

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
This method is called just once, from main function at povray.cpp

Creates a list with NUMBER_OF_PRIOQS positions, each one holding a dynamic
array of MAX_NUMBER_OF_INTERSECTIONS Intersections.
*/
PriorityQueueNode *
pq_init()
{
    int i;
    PriorityQueueNode *newNode;
    PriorityQueueNode *head;

    head = nullptr;

    for (i = 0; i < NUMBER_OF_PRIOQS; i++) {
        newNode = new PriorityQueueNode();
        if (newNode == nullptr) {
            fprintf(stderr, "\nOut of memory. Cannot allocate queues");
            close_all();
            exit(1);
        }

        newNode->next_pq = head;
        head = newNode;

        newNode->queue = new Intersection[MAX_NUMBER_OF_INTERSECTIONS];
        if (newNode->queue == nullptr) {
            fprintf(stderr, "\nOut of memory. Cannot allocate queue entries");
            close_all();
            exit(1);
        }
        newNode->current_entry = 0;
        newNode->queue_size = 0;
    }

    return head;
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

PriorityQueueNode *
pq_pop(int indexSize)
{
    if (indexSize >= MAX_NUMBER_OF_INTERSECTIONS) {
        indexSize = MAX_NUMBER_OF_INTERSECTIONS - 1;
    }

    //- pq_alloc ---------------------------------------------------------------
    PriorityQueueNode *pq;

    if (GLOBAL_priorityQueuesHead == nullptr) {
        fprintf(stderr, "\nOut of prioqs");
        close_all();
        exit(1);
    }
    pq = GLOBAL_priorityQueuesHead;

    //--------------------------------------------------------------------------
    if (pq == nullptr) {
        return nullptr;
    }

    GLOBAL_priorityQueuesHead = pq->next_pq;
    pq->queue_size = indexSize;
    pq->current_entry = 0;
    return pq;
}
