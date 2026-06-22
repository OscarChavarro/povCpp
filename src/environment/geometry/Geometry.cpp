#include "java/util/PriorityQueue.txx"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/Intersection.h"

int
Geometry::allIntersectionsForOwner(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
    SimpleBody *owner)
{
    const int sizeBefore = depthQueue->size();
    const int result = allIntersections(ray, depthQueue);
    const int newCount = depthQueue->size() - sizeBefore;
    if (newCount == 0) {
        return result;
    }

    // Every candidate this call just added carries hitGeometry set to the
    // bare primitive (this) directly and type-safely at the point of
    // creation - no sentinel cast needed here, unlike the old
    // ownerSimpleBody backfill this replaces. Only the owner pointer itself
    // needs deferred backfill; resolving its material is left to the render
    // layer, so this geometry-layer file never needs SimpleBody's definition.
    int updated = 0;
    for (Intersection &candidate : *depthQueue) {
        if (candidate.getHitGeometry() == this) {
            candidate.setOwner(owner);
            if (++updated == newCount) {
                break;
            }
        }
    }
    return result;
}
