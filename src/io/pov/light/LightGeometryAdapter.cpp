#include "io/pov/light/LightGeometryAdapter.h"

int
LightGeometryAdapter::doIntersectionForAllRayCrossings(
    RayWithSegments *ray,
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

void
LightGeometryAdapter::translateGeometry(Vector3Dd *vector)
{
    if (getLight() != nullptr) {
        getLight()->translate(vector);
    }
}

void
LightGeometryAdapter::rotateGeometry(Vector3Dd *vector)
{
    if (getLight() != nullptr) {
        getLight()->rotate(vector);
    }
}

void
LightGeometryAdapter::scaleGeometry(Vector3Dd *vector)
{
    if (getLight() != nullptr) {
        getLight()->scale(vector);
    }
}

void
LightGeometryAdapter::invertGeometry()
{
    if (getLight() != nullptr) {
        getLight()->invert();
    }
}
