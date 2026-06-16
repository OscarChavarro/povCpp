#include "environment/scene/TranslatedBody.h"
#include "environment/geometry/Intersection.h"
#include "environment/material/MaterialUtils.h"

int
TranslatedBody::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    // The wrapped geometry stamps each intersection it produces with its own
    // address (a sentinel) as the Shape. Rewrite just those entries to point at
    // this TranslatedBody, so the shader can reach the material/colour we own.
    // Sibling entries already wrapped by other bodies carry a different Shape
    // pointer and are left untouched.
    const int result = geometry->allIntersections(ray, depthQueue);
    TranslatedBody * const sentinel = reinterpret_cast<TranslatedBody *>(geometry);
    for (Intersection &candidate : *depthQueue) {
        if (candidate.Shape == sentinel) {
            candidate.Shape = this;
        }
    }
    return result;
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
    TranslatedBody *newBody = new TranslatedBody;
    *newBody = *this;
    newBody->geometry = (Geometry *)geometry->copy();
    if (newBody->material != nullptr) {
        newBody->material = MaterialUtils::instance().copyTexture(newBody->material);
    }
    return (void *)newBody;
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
#include "java/util/PriorityQueue.txx"
