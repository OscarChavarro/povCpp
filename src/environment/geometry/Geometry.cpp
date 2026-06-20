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

    SimpleBody * const sentinel = reinterpret_cast<SimpleBody *>(this);
    int updated = 0;
    for (Intersection &candidate : *depthQueue) {
        if (candidate.getOwnerSimpleBody() == sentinel) {
            candidate.setOwnerSimpleBody(owner);
            if (++updated == newCount) {
                break;
            }
        }
    }
    return result;
}
