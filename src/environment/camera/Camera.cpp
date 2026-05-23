/****************************************************************************
 *                     viewpnt.c
 *
 *  This module implements methods for managing the viewpoint.
 *
 *****************************************************************************/

#include "environment/camera/Camera.h"
Methods cameraMethods = {nullptr, nullptr, nullptr, nullptr,
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
    this->methods = (Methods *)&cameraMethods;
    this->Type = VIEWPOINT_TYPE;
    *&this->Location = Vector3Dd(0.0, 0.0, 0.0);
    *&this->Direction = Vector3Dd(0.0, 0.0, 1.0);
    *&this->Up = Vector3Dd(0.0, 1.0, 0.0);
    *&this->Right = Vector3Dd(1.33, 0.0, 0.0);
    *&this->Sky = Vector3Dd(0.0, 1.0, 0.0);
}

void
Camera::translateCamera(SimpleBody *object, Vector3Dd *vector)
{
    VectorOps::vAdd(((Camera *)object)->Location,
        ((Camera *)object)->Location, *vector);
}

void
Camera::rotateCamera(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    Camera *viewpoint = (Camera *)object;

    Transformation::getRotationTransformation(&transformation, vector);
    Transformation::MTransformVector(
        &(viewpoint->Location), &(viewpoint->Location), &transformation);

    Transformation::MTransformVector(
        &(viewpoint->Direction), &(viewpoint->Direction), &transformation);

    Transformation::MTransformVector(
        &(viewpoint->Up), &(viewpoint->Up), &transformation);

    Transformation::MTransformVector(
        &(viewpoint->Right), &(viewpoint->Right), &transformation);
}

void
Camera::scaleCamera(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    Camera *viewpoint = (Camera *)object;

    Transformation::getScalingTransformation(&transformation, vector);
    Transformation::MTransformVector(
        &(viewpoint->Location), &(viewpoint->Location), &transformation);

    Transformation::MTransformVector(
        &(viewpoint->Direction), &(viewpoint->Direction), &transformation);

    Transformation::MTransformVector(
        &(viewpoint->Up), &(viewpoint->Up), &transformation);

    Transformation::MTransformVector(
        &(viewpoint->Right), &(viewpoint->Right), &transformation);
}
