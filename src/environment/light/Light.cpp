/**
light.c

This module implements the point & spot light source primitive.
*/

#include "environment/light/Light.h"

Light::Light() :
    Light(nullptr, Vector3Dd(0.0, 0.0, 0.0), Vector3Dd(0.0, 0.0, 1.0), false,
        10.0, 0.35, 0.35)
{
}

Light::Light(const Vector3Dd &center, const Vector3Dd &pointsAt, bool inverted,
    double coefficient, double radius, double falloff) :
    Light(nullptr, center, pointsAt, inverted, coefficient, radius, falloff)
{
}

Light::Light(const ColorRgba *shapeColor, const Vector3Dd &center,
    const Vector3Dd &pointsAt, bool inverted, double coefficient,
    double radius, double falloff) :
    shapeColor(shapeColor != nullptr ? new ColorRgba(*shapeColor) : nullptr),
    center(center),
    pointsAt(pointsAt),
    inverted(inverted),
    coefficient(coefficient),
    radius(radius),
    falloff(falloff)
{
}

Light::~Light()
{
    delete shapeColor;
}

void
Light::applyLinearTransformation(const Matrix4x4d &transformation)
{
    this->getCenter() = transformation.transpose().multiply(this->getCenter());
    this->getPointsAt() = transformation.transpose().multiply(this->getPointsAt());
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
    Matrix4x4d transformation = Matrix4x4d().scale(
        vector->x(), vector->y(), vector->z());
    applyLinearTransformation(transformation);
}

void
Light::invert()
{
    inverted = !inverted;
}
