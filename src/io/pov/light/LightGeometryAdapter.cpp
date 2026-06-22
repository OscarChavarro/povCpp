#include "io/pov/light/LightGeometryAdapter.h"

int
LightGeometryAdapter::allIntersections(
    RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    (void)ray;
    (void)depthQueue;
    return false;
}

int
LightGeometryAdapter::allIntersectionsForMaterial(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
    Material *material)
{
    (void)ray;
    (void)depthQueue;
    (void)material;
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
