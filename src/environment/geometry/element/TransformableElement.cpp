#include "java/util/PriorityQueue.txx"
#include "common/dataStructures/IntersectionPriorityQueuePool.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/element/TransformableElement.h"

bool
TransformableElement::intersect(RayWithSegments *ray, Intersection &out)
{
    java::PriorityQueue<Intersection> * const depthQueue =
        ray->getIntersectionQueuePool()->pop(128);
    bool hit = false;
    if (allIntersections(ray, depthQueue) && depthQueue->size() > 0) {
        out = depthQueue->peek();
        hit = true;
    }
    ray->getIntersectionQueuePool()->push(depthQueue);
    return hit;
}
