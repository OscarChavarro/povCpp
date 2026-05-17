/****************************************************************************
 *                     PriorityQueuePool.cpp
 *
 *  This module implements priority queue pool management.
 *
 ****************************************************************************/

#include "environment/geometry/PriorityQueuePool.h"
#include "environment/geometry/Intersection.h"
#include "common/FrameConfig.h"
#include "app/PovApp.h"

//===========================================================================

// used on pq_new and pq_init
static constexpr int NUMBER_OF_PRIOQS = 32;
static constexpr int MAX_NUMBER_OF_INTERSECTIONS = 128;

//===========================================================================

extern PriorityQueueNode *GLOBAL_priorityQueuesHead;

/**
This method is called just once, from main function at povray.cpp

Creates a list with NUMBER_OF_PRIOQS positions, each one holding a dynamic
array of MAX_NUMBER_OF_INTERSECTIONS Intersections.
*/
PriorityQueueNode *
PriorityQueuePool::pqInit()
{
    int i;
    PriorityQueueNode *newNode;
    PriorityQueueNode *head;

    head = nullptr;

    for (i = 0; i < NUMBER_OF_PRIOQS; i++) {
        newNode = new PriorityQueueNode();
        if (newNode == nullptr) {
            fprintf(stderr, "\nOut of memory. Cannot allocate queues");
            PovApp::closeAll();
            exit(1);
        }

        newNode->next_pq = head;
        head = newNode;

        newNode->queue = new Intersection[MAX_NUMBER_OF_INTERSECTIONS];
        if (newNode->queue == nullptr) {
            fprintf(stderr, "\nOut of memory. Cannot allocate queue entries");
            PovApp::closeAll();
            exit(1);
        }
        newNode->current_entry = 0;
        newNode->queue_size = 0;
    }

    return head;
}

PriorityQueueNode *
PriorityQueuePool::pqPop(int indexSize)
{
    if (indexSize >= MAX_NUMBER_OF_INTERSECTIONS) {
        indexSize = MAX_NUMBER_OF_INTERSECTIONS - 1;
    }

    //- pq_alloc ---------------------------------------------------------------
    PriorityQueueNode *pq;

    if (GLOBAL_priorityQueuesHead == nullptr) {
        fprintf(stderr, "\nOut of prioqs");
        PovApp::closeAll();
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
