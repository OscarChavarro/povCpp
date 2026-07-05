#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/Geometry.h"

void
Geometry::applyAnnotatedEmissionContext(
    IntersectionCandidate &candidate,
    const GeometryIntersectionEmissionContext &context)
{
    IntersectionAttributes &attributes = candidate.getAttributes();
    attributes.pushDetailOwner(context.detailOwner);
    attributes.setMaterialUsesObjectLocalPoint(context.materialUsesObjectLocalPoint);
}

bool
Geometry::doIntersectionFirstHitViaCrossings(RayWithTracingState *ray, IntersectionCandidate &out)
{
    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (doIntersectionForAllRayCrossings(ray, depthQueue) && depthQueue->size() > 0) {
        out = depthQueue->peek();
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}

int
Geometry::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    if (depthQueue->size() == 0) {
        const int found =
            doIntersectionForAllRayCrossings(ray, depthQueue, context.materialOverride);
        if (!found || depthQueue->size() == 0) {
            return found;
        }
        for (IntersectionCandidate &candidate : *depthQueue) {
            applyAnnotatedEmissionContext(candidate, context);
        }
        return found;
    }

    java::PriorityQueue<IntersectionCandidate> * const freshQueue =
        ray->getIntersectionQueuePool()->pop(128);
    const int found =
        doIntersectionForAllRayCrossings(ray, freshQueue, context.materialOverride);
    if (found && freshQueue->size() > 0) {
        for (IntersectionCandidate &candidate : *freshQueue) {
            applyAnnotatedEmissionContext(candidate, context);
            depthQueue->offer(candidate);
        }
    }
    ray->getIntersectionQueuePool()->push(freshQueue);
    return found;
}

void
Geometry::doExtraInformation(const RayWithTracingState &ray, double t, PovRayHit *hit)
{
    (void)t;
    if (hit->needsNormal()) {
        computeSurfaceNormal(&hit->n, &hit->p, ray.getConfig());
    }
}
