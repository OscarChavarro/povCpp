/****************************************************************************
 *                     viewpnt.c
 *
 *  This module implements methods for managing the viewpoint.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "geom/ViewPnt.h"

/*===========================================================================*/

Methods Viewpoint_Methods = {nullptr, nullptr, nullptr, nullptr, copyViewpoint,
    translateViewpoint, rotateViewpoint, scaleViewpoint, nullptr};

/*===========================================================================*/

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
