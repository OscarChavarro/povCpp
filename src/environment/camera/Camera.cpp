#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/camera/Camera.h"

Camera::Camera() :
    Camera(Vector3Dd(0.0, 0.0, 0.0), Vector3Dd(0.0, 0.0, 1.0),
        Vector3Dd(0.0, 1.0, 0.0), Vector3Dd(1.33, 0.0, 0.0),
        Vector3Dd(0.0, 1.0, 0.0))
{
}

Camera::Camera(const Vector3Dd &location, const Vector3Dd &direction,
    const Vector3Dd &up, const Vector3Dd &right, const Vector3Dd &sky) :
    location(location),
    direction(direction),
    up(up),
    right(right),
    sky(sky)
{
}

void
Camera::applyLinearTransformation(const Matrix4x4d &transformation)
{
    this->getLocation() = transformation.transpose().multiply(this->getLocation());
    this->getDirection() = transformation.transpose().multiply(this->getDirection());
    this->getUp() = transformation.transpose().multiply(this->getUp());
    this->getRight() = transformation.transpose().multiply(this->getRight());
}

void *
Camera::copy()
{
    const Camera *viewpoint = this;
    Camera * const newViewpoint = new Camera(
        viewpoint->getLocation(), viewpoint->getDirection(), viewpoint->getUp(),
        viewpoint->getRight(), viewpoint->getSky());
    if (newViewpoint == nullptr) {
        return nullptr;
    }
    return (newViewpoint);
}

void
Camera::translate(Vector3Dd *vector)
{
    this->getLocation() = this->getLocation().add(*vector);
}

void
Camera::rotate(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    applyLinearTransformation(transformation);
}

void
Camera::scale(Vector3Dd *vector)
{
    Matrix4x4d transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    applyLinearTransformation(transformation);
}
