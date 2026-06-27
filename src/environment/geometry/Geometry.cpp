#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/common/memoryManagement/MemoryPool.txx"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/Geometry.h"

bool
Geometry::doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out)
{
    java::PriorityQueue<IntersectionCandidate> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (allIntersections(ray, depthQueue) && depthQueue->size() > 0) {
        out = depthQueue->peek();
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}

void
Geometry::doExtraInformation(const RayWithSegments &ray, double t, PovRayHit *hit)
{
    (void)t;
    normal(&hit->n, &hit->p, ray.getConfig());
}

int
Geometry::allIntersectionsForMaterial(
    RayWithSegments *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *material)
{
    const int sizeBefore = depthQueue->size();
    const int result = allIntersections(ray, depthQueue);
    const int newCount = depthQueue->size() - sizeBefore;
    if (newCount == 0) {
        return result;
    }

    // Every candidate this call just added carries hitGeometry set to the
    // bare primitive (this) directly and type-safely at the point of
    // creation - no sentinel cast needed here. Only the material pointer
    // itself needs deferred backfill.
    int updated = 0;
    for (IntersectionCandidate &candidate : *depthQueue) {
        if (candidate.getAttributes().getHitGeometry() == this) {
            candidate.getAttributes().setMaterial(material);
            if (++updated == newCount) {
                break;
            }
        }
    }
    return result;
}
