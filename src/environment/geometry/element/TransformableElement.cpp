#include "java/util/PriorityQueue.txx"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/PovRayHit.h"
#include "environment/geometry/element/TransformableElement.h"

bool
TransformableElement::doIntersectionFirstHit(RayWithSegments *ray, IntersectionCandidate &out)
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
TransformableElement::doExtraInformation(const RayWithSegments &ray, double t, PovRayHit *hit)
{
    (void)t;
    normal(&hit->n, &hit->p, ray.getConfig());
}
