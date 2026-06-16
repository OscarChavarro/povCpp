#include "environment/geometry/Intersection.h"

#include "environment/material/MaterialUtils.h"

#include "environment/scene/TranslatedBody.h"

int
TranslatedBody::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    // The wrapped geometry stamps each intersection it produces with its own
    // address (a sentinel) as the Shape. Rewrite just those entries to point at
    // this TranslatedBody, so the shader can reach the material/colour we own.
    // Sibling entries already wrapped by other bodies carry a different Shape
    // pointer and are left untouched.
    //
    // Optimization: record queue size before and after, then early-exit the
    // scan once we have updated exactly that many entries. Avoids scanning
    // sibling entries that can never match this sentinel.
    const int sizeBefore = depthQueue->size();
    const int result = geometry->allIntersections(ray, depthQueue);
    const int newCount = depthQueue->size() - sizeBefore;
    if (newCount == 0) {
        return result;
    }
    TranslatedBody * const sentinel = reinterpret_cast<TranslatedBody *>(geometry);
    int updated = 0;
    for (Intersection &candidate : *depthQueue) {
        if (candidate.Shape == sentinel) {
            candidate.Shape = this;
            if (++updated == newCount) {
                break;
            }
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
    const Matrix4x4d delta = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    const Matrix4x4d deltaInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    transform = transform.multiply(delta);
    transformInverse = deltaInverse.multiply(transformInverse);
}

void
TranslatedBody::rotate(Vector3Dd *vector)
{
    geometry->rotateGeometry(vector);
    MaterialUtils::instance().rotateTexture(&material, vector);
    Matrix4x4d delta;
    Matrix4x4d deltaInverse;
    delta.axisRotationRodrigues(&deltaInverse, vector);
    transform = transform.multiply(delta);
    transformInverse = deltaInverse.multiply(transformInverse);
}

void
TranslatedBody::scale(Vector3Dd *vector)
{
    geometry->scaleGeometry(vector);
    MaterialUtils::instance().scaleTexture(&material, vector);
    const Matrix4x4d delta = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    const Matrix4x4d deltaInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    transform = transform.multiply(delta);
    transformInverse = deltaInverse.multiply(transformInverse);
}

void
TranslatedBody::invert()
{
    geometry->invertGeometry();
}

#include "java/util/PriorityQueue.txx"
