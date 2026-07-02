#ifndef __CSG_SCRATCH_CONTEXT__
#define __CSG_SCRATCH_CONTEXT__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "java/util/PriorityQueue.h"

class CsgScratchContext {
public:
    static constexpr int MAX_SCRATCH_QUEUES = 8;

    explicit CsgScratchContext(RayWithSegments *ownerRayArg)
        : used(0), ownerRay(ownerRayArg)
    {
        for (int i = 0; i < MAX_SCRATCH_QUEUES; i++) {
            queues[i] = nullptr;
        }
    }

    java::PriorityQueue<IntersectionCandidate> *borrowQueue()
    {
        if (used >= MAX_SCRATCH_QUEUES) {
            return ownerRay->getIntersectionQueuePool()->pop(128);
        }
        if (queues[used] == nullptr) {
            queues[used] = ownerRay->getIntersectionQueuePool()->pop(128);
        }
        java::PriorityQueue<IntersectionCandidate> *queue = queues[used++];
        queue->clear();
        return queue;
    }

    void returnQueue(java::PriorityQueue<IntersectionCandidate> *queue)
    {
        queue->clear();
        if (used > 0 && queues[used - 1] == queue) {
            used--;
            return;
        }
        ownerRay->getIntersectionQueuePool()->push(queue);
    }

    void releaseAll()
    {
        for (int i = MAX_SCRATCH_QUEUES - 1; i >= 0; i--) {
            if (queues[i] != nullptr) {
                ownerRay->getIntersectionQueuePool()->push(queues[i]);
            }
        }
    }

private:
    java::PriorityQueue<IntersectionCandidate> *queues[MAX_SCRATCH_QUEUES];
    int used;
    RayWithSegments *ownerRay;
};

#endif
