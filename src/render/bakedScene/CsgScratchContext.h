#ifndef __CSG_SCRATCH_CONTEXT__
#define __CSG_SCRATCH_CONTEXT__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "java/util/PriorityQueue.h"
#include "render/raySharedCache/RaySharedCache.h"

class CsgScratchContext {
public:
    static constexpr int MAX_SCRATCH_QUEUES = 8;

    CsgScratchContext(RayWithTracingState *ownerRayArg, RaySharedCache *cacheArg)
        : used(0), ownerRay(ownerRayArg), cache(cacheArg)
    {
        for (int i = 0; i < MAX_SCRATCH_QUEUES; i++) {
            queues[i] = nullptr;
        }
    }

    RaySharedCache &getCache() { return *cache; }

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
    RayWithTracingState *ownerRay;
    RaySharedCache *cache;
};

#endif
