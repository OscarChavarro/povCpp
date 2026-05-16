/****************************************************************************
 *                     viewpnt.c
 *
 *  This module implements methods for managing the viewpoint.
 *
 *****************************************************************************/

#include "geom/ViewPnt.h"
Methods Viewpoint_Methods = {nullptr, nullptr, nullptr, nullptr, copyViewpoint,
    translateViewpoint, rotateViewpoint, scaleViewpoint, nullptr};
void *
copyViewpoint(SimpleBody *object)
{
    Viewpoint *viewpoint = (Viewpoint *)object;
    Viewpoint *newViewpoint;

    newViewpoint = getViewpoint();

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
    makeVector(&this->Location, 0.0, 0.0, 0.0);
    makeVector(&this->Direction, 0.0, 0.0, 1.0);
    makeVector(&this->Up, 0.0, 1.0, 0.0);
    makeVector(&this->Right, 1.33, 0.0, 0.0);
    makeVector(&this->Sky, 0.0, 1.0, 0.0);
}

void
translateViewpoint(SimpleBody *object, Vector3D *vector)
{
    VAdd(((Viewpoint *)object)->Location, ((Viewpoint *)object)->Location,
        *vector);
}

void
rotateViewpoint(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    Viewpoint *viewpoint = (Viewpoint *)object;

    getRotationTransformation(&transformation, vector);
    MTransformVector(
        &(viewpoint->Location), &(viewpoint->Location), &transformation);

    MTransformVector(
        &(viewpoint->Direction), &(viewpoint->Direction), &transformation);

    MTransformVector(&(viewpoint->Up), &(viewpoint->Up), &transformation);

    MTransformVector(&(viewpoint->Right), &(viewpoint->Right), &transformation);
}

void
scaleViewpoint(SimpleBody *object, Vector3D *vector)
{
    Transformation transformation;
    Viewpoint *viewpoint = (Viewpoint *)object;

    getScalingTransformation(&transformation, vector);
    MTransformVector(
        &(viewpoint->Location), &(viewpoint->Location), &transformation);

    MTransformVector(
        &(viewpoint->Direction), &(viewpoint->Direction), &transformation);

    MTransformVector(&(viewpoint->Up), &(viewpoint->Up), &transformation);

    MTransformVector(&(viewpoint->Right), &(viewpoint->Right), &transformation);
}
