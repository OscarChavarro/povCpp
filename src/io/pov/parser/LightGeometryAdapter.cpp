#include "io/pov/parser/LightGeometryAdapter.h"

int
LightGeometryAdapter::allIntersections(
    RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    (void)ray;
    (void)depthQueue;
    return false;
}

int
LightGeometryAdapter::allIntersectionsForOwner(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
    SimpleBody *owner)
{
    (void)ray;
    (void)depthQueue;
    (void)owner;
    return false;
}

int
LightGeometryAdapter::inside(Vector3Dd *point)
{
    (void)point;
    return false;
}

void *
LightGeometryAdapter::copy()
{
    return new LightGeometryAdapter(getLight() != nullptr ? getLight()->copy() : nullptr);
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
