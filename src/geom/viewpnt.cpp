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

#include "geom/viewpnt.h"

/*===========================================================================*/

Methods Viewpoint_Methods =
{ NULL, NULL, NULL, NULL, Copy_Viewpoint,
    Translate_Viewpoint, Rotate_Viewpoint,
    Scale_Viewpoint, NULL};

/*===========================================================================*/

void *
Copy_Viewpoint(SimpleBody *Object)
{
    Viewpoint *viewpoint = (Viewpoint *) Object;
    Viewpoint *New_Viewpoint;

    New_Viewpoint = Get_Viewpoint();

    New_Viewpoint -> Location = viewpoint -> Location;
    New_Viewpoint -> Direction = viewpoint -> Direction;
    New_Viewpoint -> Right = viewpoint -> Right;
    New_Viewpoint -> Up = viewpoint -> Up;
    return (New_Viewpoint);
}

void
Translate_Viewpoint(SimpleBody *Object, Vector3D *Vector)
{
    VAdd (((Viewpoint *) Object) -> Location, 
        ((Viewpoint *) Object) -> Location,
        *Vector);
}

void
Rotate_Viewpoint(SimpleBody *Object, Vector3D *Vector)
{
    Transformation transformation;
    Viewpoint *viewpoint = (Viewpoint *) Object;

    Get_Rotation_Transformation(&transformation, Vector);
    MTransformVector (&(viewpoint -> Location),
        &(viewpoint -> Location), &transformation);

    MTransformVector (&(viewpoint -> Direction),
        &(viewpoint -> Direction), &transformation);

    MTransformVector (&(viewpoint -> Up),
        &(viewpoint -> Up), &transformation);

    MTransformVector (&(viewpoint -> Right),
        &(viewpoint -> Right), &transformation);
}

void
Scale_Viewpoint(SimpleBody *Object, Vector3D *Vector)
{
    Transformation transformation;
    Viewpoint *viewpoint = (Viewpoint *) Object;

    Get_Scaling_Transformation(&transformation, Vector);
    MTransformVector (&(viewpoint -> Location),
        &(viewpoint -> Location), &transformation);

    MTransformVector (&(viewpoint -> Direction),
        &(viewpoint -> Direction), &transformation);

    MTransformVector (&(viewpoint -> Up),
        &(viewpoint -> Up), &transformation);

    MTransformVector (&(viewpoint -> Right),
        &(viewpoint -> Right), &transformation);
}
