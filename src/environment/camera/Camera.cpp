#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/camera/Camera.h"

void
Camera::applyLinearTransformation(const Matrix4x4d &transformation)
{
    this->location = transformation.transpose().multiply(this->location);
    this->direction = transformation.transpose().multiply(this->direction);
    this->up = transformation.transpose().multiply(this->up);
    this->right = transformation.transpose().multiply(this->right);
}

void
Camera::initializeDefaults()
{
    *&this->location = Vector3Dd(0.0, 0.0, 0.0);
    *&this->direction = Vector3Dd(0.0, 0.0, 1.0);
    *&this->up = Vector3Dd(0.0, 1.0, 0.0);
    *&this->right = Vector3Dd(1.33, 0.0, 0.0);
    *&this->sky = Vector3Dd(0.0, 1.0, 0.0);
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

    newViewpoint->location = viewpoint->location;
    newViewpoint->direction = viewpoint->direction;
    newViewpoint->right = viewpoint->right;
    newViewpoint->up = viewpoint->up;
    return (newViewpoint);
}

void
Camera::translate(Vector3Dd *vector)
{
    this->location = this->location.add(*vector);
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
