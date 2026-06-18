/**
light.c

This module implements the point & spot light source primitive.
*/

#include "environment/light/Light.h"

Light::Light() :
    Light(Vector3Dd(0.0, 0.0, 0.0), Vector3Dd(0.0, 0.0, 1.0), false,
        10.0, 0.35, 0.35)
{
}

Light::Light(const Vector3Dd &center, const Vector3Dd &pointsAt, bool inverted,
    double coefficient, double radius, double falloff) :
    center(center),
    pointsAt(pointsAt),
    nextLightSource(nullptr),
    inverted(inverted),
    coefficient(coefficient),
    radius(radius),
    falloff(falloff)
{
}

void
Light::applyLinearTransformation(const Matrix4x4d &transformation)
{
    this->getCenter() = transformation.transpose().multiply(this->getCenter());
    this->getPointsAt() = transformation.transpose().multiply(this->getPointsAt());
}

void
Light::copyStateInto(Light *dst) const
{
    *dst = *this;
}

void
Light::translate(Vector3Dd *vector)
{
    this->getCenter() = this->getCenter().add(*vector);
    this->getPointsAt() = this->getPointsAt().add(*vector);
}

void
Light::rotate(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    applyLinearTransformation(transformation);
}

void
Light::scale(Vector3Dd *vector)
{
    Matrix4x4d transformation;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    applyLinearTransformation(transformation);
}

void
Light::invert()
{
    this->setInverted(!this->isInverted());
}
