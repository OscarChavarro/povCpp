/**
viewpnt.c

This module implements methods for managing the viewpoint.
*/

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "environment/camera/Camera.h"

void
Camera::initializeDefaults()
{
    this->type = GeometryTypes::VIEWPOINT_TYPE;
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
    Camera * const viewpoint = this;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    viewpoint->location = transformation.transpose().multiply(viewpoint->location);
    viewpoint->direction = transformation.transpose().multiply(viewpoint->direction);
    viewpoint->up = transformation.transpose().multiply(viewpoint->up);
    viewpoint->right = transformation.transpose().multiply(viewpoint->right);
}

void
Camera::scale(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Camera * const viewpoint = this;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    viewpoint->location = transformation.transpose().multiply(viewpoint->location);
    viewpoint->direction = transformation.transpose().multiply(viewpoint->direction);
    viewpoint->up = transformation.transpose().multiply(viewpoint->up);
    viewpoint->right = transformation.transpose().multiply(viewpoint->right);
}
#include "java/util/PriorityQueue.txx"
