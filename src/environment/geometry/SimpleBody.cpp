#include "environment/geometry/Intersection.h"

#include "environment/material/MaterialUtils.h"

#include "environment/geometry/SimpleBody.h"

int
SimpleBody::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    return geometry->allIntersectionsForOwner(ray, depthQueue, this);
}

int
SimpleBody::inside(Vector3Dd *point)
{
    return geometry->inside(point);
}

void
SimpleBody::normal(Vector3Dd *result, Vector3Dd *intersectionPoint)
{
    geometry->normal(result, intersectionPoint);
}

void *
SimpleBody::copy()
{
    SimpleBody *newBody = new SimpleBody;
    *newBody = *this;
    newBody->geometry = (Geometry *)geometry->copy();
    if (newBody->material != nullptr) {
        newBody->material = MaterialUtils::instance().copyTexture(newBody->material);
    }
    return (void *)newBody;
}

void
SimpleBody::translate(Vector3Dd *vector)
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
SimpleBody::rotate(Vector3Dd *vector)
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
SimpleBody::scale(Vector3Dd *vector)
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
SimpleBody::invert()
{
    geometry->invertGeometry();
}

#include "java/util/PriorityQueue.txx"
