/**
viewpnt.c

This module implements methods for managing the viewpoint.
*/

#include "environment/camera/Camera.h"

void
Camera::initializeDefaults()
{
    this->Type = GeometryTypes::VIEWPOINT_TYPE;
    *&this->Location = Vector3Dd(0.0, 0.0, 0.0);
    *&this->Direction = Vector3Dd(0.0, 0.0, 1.0);
    *&this->Up = Vector3Dd(0.0, 1.0, 0.0);
    *&this->Right = Vector3Dd(1.33, 0.0, 0.0);
    *&this->Sky = Vector3Dd(0.0, 1.0, 0.0);
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

    newViewpoint->Location = viewpoint->Location;
    newViewpoint->Direction = viewpoint->Direction;
    newViewpoint->Right = viewpoint->Right;
    newViewpoint->Up = viewpoint->Up;
    return (newViewpoint);
}

void
Camera::translate(Vector3Dd *vector)
{
    this->Location = this->Location.add(*vector);
}

void
Camera::rotate(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;
    Camera * const viewpoint = this;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    viewpoint->Location = transformation.transpose().multiply(viewpoint->Location);
    viewpoint->Direction = transformation.transpose().multiply(viewpoint->Direction);
    viewpoint->Up = transformation.transpose().multiply(viewpoint->Up);
    viewpoint->Right = transformation.transpose().multiply(viewpoint->Right);
}

void
Camera::scale(Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Camera * const viewpoint = this;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    viewpoint->Location = transformation.transpose().multiply(viewpoint->Location);
    viewpoint->Direction = transformation.transpose().multiply(viewpoint->Direction);
    viewpoint->Up = transformation.transpose().multiply(viewpoint->Up);
    viewpoint->Right = transformation.transpose().multiply(viewpoint->Right);
}
#include "java/util/PriorityQueue.txx"
