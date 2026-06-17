/**
light.c

This module implements the point & spot light source primitive.
*/

#include "environment/light/Light.h"

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
