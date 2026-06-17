#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/camera/Camera.h"

void
Camera::applyLinearTransformation(const Matrix4x4d &transformation)
{
    this->getLocation() = transformation.transpose().multiply(this->getLocation());
    this->getDirection() = transformation.transpose().multiply(this->getDirection());
    this->getUp() = transformation.transpose().multiply(this->getUp());
    this->getRight() = transformation.transpose().multiply(this->getRight());
}

void
Camera::initializeDefaults()
{
    this->getLocation() = Vector3Dd(0.0, 0.0, 0.0);
    this->getDirection() = Vector3Dd(0.0, 0.0, 1.0);
    this->getUp() = Vector3Dd(0.0, 1.0, 0.0);
    this->getRight() = Vector3Dd(1.33, 0.0, 0.0);
    this->getSky() = Vector3Dd(0.0, 1.0, 0.0);
}

void *
Camera::copy()
{
    const Camera *viewpoint = this;
    Camera * const newViewpoint = new Camera();
    if (newViewpoint == nullptr) {
        return nullptr;
    }
    newViewpoint->initializeDefaults();

    newViewpoint->getLocation() = viewpoint->getLocation();
    newViewpoint->getDirection() = viewpoint->getDirection();
    newViewpoint->getRight() = viewpoint->getRight();
    newViewpoint->getUp() = viewpoint->getUp();
    newViewpoint->getSky() = viewpoint->getSky();
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
    Matrix4x4d transformation;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    applyLinearTransformation(transformation);
}
