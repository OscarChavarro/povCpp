/****************************************************************************
 *                     viewpnt.c
 *
 *  This module implements methods for managing the viewpoint.
 *
 *****************************************************************************/

#include "environment/camera/Viewpoint.h"
#include "io/pov/SceneFactory.h"
Methods Viewpoint_Methods = {nullptr, nullptr, nullptr, nullptr,
    Viewpoint::copyViewpoint, Viewpoint::translateViewpoint,
    Viewpoint::rotateViewpoint, Viewpoint::scaleViewpoint, nullptr};
void *
Viewpoint::copyViewpoint(SimpleBody *object)
{
    Viewpoint *viewpoint = (Viewpoint *)object;
    Viewpoint *newViewpoint;

    newViewpoint = SceneFactory::getViewpoint();

    newViewpoint->Location = viewpoint->Location;
    newViewpoint->Direction = viewpoint->Direction;
    newViewpoint->Right = viewpoint->Right;
    newViewpoint->Up = viewpoint->Up;
    return (newViewpoint);
}

void
Viewpoint::initializeDefaults()
{
    this->methods = (Methods *)&Viewpoint_Methods;
    this->Type = VIEWPOINT_TYPE;
    *&this->Location = Vector3Dd(0.0, 0.0, 0.0);
    *&this->Direction = Vector3Dd(0.0, 0.0, 1.0);
    *&this->Up = Vector3Dd(0.0, 1.0, 0.0);
    *&this->Right = Vector3Dd(1.33, 0.0, 0.0);
    *&this->Sky = Vector3Dd(0.0, 1.0, 0.0);
}

void
Viewpoint::translateViewpoint(SimpleBody *object, Vector3Dd *vector)
{
    VectorOps::vAdd(((Viewpoint *)object)->Location,
        ((Viewpoint *)object)->Location, *vector);
}

void
Viewpoint::rotateViewpoint(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    Viewpoint *viewpoint = (Viewpoint *)object;

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
Viewpoint::scaleViewpoint(SimpleBody *object, Vector3Dd *vector)
{
    Transformation transformation;
    Viewpoint *viewpoint = (Viewpoint *)object;

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
