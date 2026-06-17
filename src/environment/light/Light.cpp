/**
light.c

This module implements the point & spot light source primitive.
*/

#include "environment/light/Light.h"

void
Light::applyLinearTransformation(const Matrix4x4d &transformation)
{
    this->center = transformation.transpose().multiply(this->center);
    this->pointsAt = transformation.transpose().multiply(this->pointsAt);
}

void
Light::copyStateInto(Light *dst) const
{
    *dst = *this;
    if (dst->material != nullptr) {
        dst->material = dst->material->copy();
    }
}

void
Light::translate(Vector3Dd *vector)
{
    this->center = this->center.add(*vector);
    this->pointsAt = this->pointsAt.add(*vector);
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
    if (this->material != nullptr) {
        this->material->scale(vector);
    }
}

void
Light::invert()
{
    this->inverted ^= true;
}
