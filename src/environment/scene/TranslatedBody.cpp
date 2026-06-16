#include "environment/scene/TranslatedBody.h"
#include "environment/material/MaterialUtils.h"

int
TranslatedBody::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    return geometry->allIntersections(ray, depthQueue);
}

int
TranslatedBody::inside(Vector3Dd *point)
{
    return geometry->inside(point);
}

void
TranslatedBody::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    geometry->normal(result, intersectionPoint);
}

void *
TranslatedBody::copy()
{
    return nullptr;
}

void
TranslatedBody::translate(Vector3Dd *vector)
{
    geometry->translateGeometry(vector);
    MaterialUtils::instance().translateTexture(&material, vector);
}

void
TranslatedBody::rotate(Vector3Dd *vector)
{
    geometry->rotateGeometry(vector);
    MaterialUtils::instance().rotateTexture(&material, vector);
}

void
TranslatedBody::scale(Vector3Dd *vector)
{
    geometry->scaleGeometry(vector);
    MaterialUtils::instance().scaleTexture(&material, vector);
}

void
TranslatedBody::invert()
{
    geometry->invertGeometry();
}
