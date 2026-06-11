/**
viewpnt.c

This module implements methods for managing the viewpoint.
*/

#include "environment/camera/Camera.h"
Methods Camera::methodTable = {nullptr, nullptr, nullptr,
    Camera::copyCamera, Camera::translateCamera,
    Camera::rotateCamera, Camera::scaleCamera, nullptr};
void *
Camera::copyCamera(SimpleBody *object)
{
    Camera *viewpoint = (Camera *)object;
    Camera *newViewpoint = new Camera();
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
Camera::initializeDefaults()
{
    this->methods = (Methods *)&Camera::methodTable;
    this->Type = GeometryTypes::VIEWPOINT_TYPE;
    *&this->Location = Vector3Dd(0.0, 0.0, 0.0);
    *&this->Direction = Vector3Dd(0.0, 0.0, 1.0);
    *&this->Up = Vector3Dd(0.0, 1.0, 0.0);
    *&this->Right = Vector3Dd(1.33, 0.0, 0.0);
    *&this->Sky = Vector3Dd(0.0, 1.0, 0.0);
}

void
Camera::translateCamera(SimpleBody *object, Vector3Dd *vector)
{
    ((Camera *)object)->Location =
        ((Camera *)object)->Location.add(*vector);
}

void
Camera::rotateCamera(SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Matrix4x4d transformationInverse;
    Camera *viewpoint = (Camera *)object;

    transformation.axisRotationRodrigues(&transformationInverse, vector);
    viewpoint->Location = transformation.transpose().multiply(viewpoint->Location);
    viewpoint->Direction = transformation.transpose().multiply(viewpoint->Direction);
    viewpoint->Up = transformation.transpose().multiply(viewpoint->Up);
    viewpoint->Right = transformation.transpose().multiply(viewpoint->Right);
}

void
Camera::scaleCamera(SimpleBody *object, Vector3Dd *vector)
{
    Matrix4x4d transformation;
    Camera *viewpoint = (Camera *)object;

    transformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    viewpoint->Location = transformation.transpose().multiply(viewpoint->Location);
    viewpoint->Direction = transformation.transpose().multiply(viewpoint->Direction);
    viewpoint->Up = transformation.transpose().multiply(viewpoint->Up);
    viewpoint->Right = transformation.transpose().multiply(viewpoint->Right);
}
