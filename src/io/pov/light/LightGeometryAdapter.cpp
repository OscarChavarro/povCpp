#include "io/pov/light/LightGeometryAdapter.h"

int
LightGeometryAdapter::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    (void)ray;
    (void)depthQueue;
    (void)materialOverride;
    return false;
}

int
LightGeometryAdapter::doContainmentTest(const Vector3Dd &point, double distanceTolerance)
{
    (void)point;
    (void)distanceTolerance;
    return OUTSIDE;
}

void *
LightGeometryAdapter::copy()
{
    return new LightGeometryAdapter(*this);
}
