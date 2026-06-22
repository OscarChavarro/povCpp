#include "java/util/PriorityQueue.txx"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/Intersection.h"

int
Geometry::allIntersectionsForMaterial(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
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
    for (Intersection &candidate : *depthQueue) {
        if (candidate.getHitGeometry() == this) {
            candidate.setMaterial(material);
            if (++updated == newCount) {
                break;
            }
        }
    }
    return result;
}
