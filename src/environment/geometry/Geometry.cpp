#include "environment/geometry/element/PovRayHit.h"
#include "java/util/PriorityQueue.txx"

void
Geometry::applyAnnotatedEmissionContext(
    IntersectionCandidate &candidate,
    const GeometryIntersectionEmissionContext &context)
{
    IntersectionAttributes &attributes = candidate.getAttributes();
    attributes.pushDetailOwner(context.detailOwner);
    attributes.setMaterialUsesObjectLocalPoint(context.materialUsesObjectLocalPoint);
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
