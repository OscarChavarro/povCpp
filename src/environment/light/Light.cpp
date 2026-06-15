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
Light::translate(Vector3Dd *vector)
{
    this->Center = this->Center.add(*vector);
    this->pointsAt = this->pointsAt.add(*vector);
}

void
Light::rotate(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    this->Center = transformation.transpose().multiply(this->Center);
    this->pointsAt = transformation.transpose().multiply(this->pointsAt);
}

void
Light::scale(Vector3Dd *vector)
{
    Matrix4x4d transformation;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    this->Center = transformation.transpose().multiply(this->Center);
    this->pointsAt = transformation.transpose().multiply(this->pointsAt);
    MaterialUtils::instance().scaleTexture(&this->material, vector);
}

void
Light::invert()
{
    this->Inverted ^= true;
}

#include "java/util/PriorityQueue.txx"
