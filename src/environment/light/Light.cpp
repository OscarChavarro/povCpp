/**
light.c

This module implements the point & spot light source primitive.
*/

#include "java/lang/Math.h"

#include "environment/light/Light.h"

#include "environment/material/MaterialUtils.h"

int
Light::allIntersections(RayWithSegments *ray, java::PriorityQueue<Intersection> *depthQueue)
{
    return (false);
}

int
Light::allIntersectionsForOwner(
    RayWithSegments *ray,
    java::PriorityQueue<Intersection> *depthQueue,
    SimpleBody *owner)
{
    (void)ray;
    (void)depthQueue;
    (void)owner;
    return (false);
}

int
Light::inside(Vector3Dd *point)
{
    return (false);
}

void
Light::copyStateInto(Light *dst) const
{
    *dst = *this;
    if (dst->material != nullptr) {
        dst->material = MaterialUtils::instance().copyTexture(dst->material);
    }
}

void
Light::translateGeometry(Vector3Dd *vector)
{
    this->center = this->center.add(*vector);
    this->pointsAt = this->pointsAt.add(*vector);
}

void
Light::translate(Vector3Dd *vector)
{
    translateGeometry(vector);
}

void
Light::rotateGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    this->center = transformation.transpose().multiply(this->center);
    this->pointsAt = transformation.transpose().multiply(this->pointsAt);
}

void
Light::rotate(Vector3Dd *vector)
{
    rotateGeometry(vector);
}

void
Light::scaleGeometry(Vector3Dd *vector)
{
    Matrix4x4d transformation;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    this->center = transformation.transpose().multiply(this->center);
    this->pointsAt = transformation.transpose().multiply(this->pointsAt);
}

void
Light::scale(Vector3Dd *vector)
{
    scaleGeometry(vector);
    MaterialUtils::instance().scaleTexture(&this->material, vector);
}

void
Light::invertGeometry()
{
    this->inverted ^= true;
}

void
Light::invert()
{
    invertGeometry();
}

#include "java/util/PriorityQueue.txx"
