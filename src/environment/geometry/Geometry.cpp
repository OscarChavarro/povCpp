#include "environment/geometry/Geometry.h"
#include "environment/geometry/element/Intersection.h"
#include "java/util/PriorityQueue.txx"

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
        if (candidate.getSimpleBody() == sentinel) {
            candidate.setSimpleBody(owner);
            if (++updated == newCount) {
                break;
            }
        }
    }
    return result;
}
