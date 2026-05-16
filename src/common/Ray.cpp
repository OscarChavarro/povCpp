/****************************************************************************
 *                     ray.c
 *
 *  This module implements the code pertaining to rays.
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

#include "common/Ray.h"
#include "common/PovProto.h"
#include "common/Vector.h"

/*===========================================================================*/

#define Mix(a, b, c)                                                           \
    {                                                                          \
        (a).x = (b).x * (c).y;                                                 \
        (a).y = (b).x * (c).z;                                                 \
        (a).z = (b).y * (c).z;                                                 \
    }

/*===========================================================================*/

void
Ray::makeRay()
{
    Vector3D tempInitDir;

    VSquareTerms(this->Initial_2, this->Initial);
    VSquareTerms(this->Direction_2, this->Direction);
    VEvaluate(this->Initial_Direction, this->Initial, this->Direction);
    Mix(this->Mixed_Initial_Initial, this->Initial, this->Initial);
    Mix(this->Mixed_Dir_Dir, this->Direction, this->Direction);
    Mix(tempInitDir, this->Initial, this->Direction);
    Mix(this->Mixed_Init_Dir, this->Direction, this->Initial);
    VAdd(this->Mixed_Init_Dir, this->Mixed_Init_Dir, tempInitDir);
    this->Quadric_Constants_Cached = TRUE;
}

void
Ray::initializeContainers()
{
    this->Containing_Index = -1;
}

void
copyRayContainers(Ray *destRay, Ray *sourceRay)
{
    register int i;

    if ((destRay->Containing_Index = sourceRay->Containing_Index) >=
        MAX_CONTAINING_OBJECTS) {
        fprintf(stderr, "ERROR - Containing Index too high\n");
        closeAll();
        exit(1);
    }

    for (i = 0; i < MAX_CONTAINING_OBJECTS; i++) {
        destRay->Containing_Textures[i] = sourceRay->Containing_Textures[i];
        destRay->Containing_IORs[i] = sourceRay->Containing_IORs[i];
    }
}

void
rayEnter(Ray *ray, Texture *texture)
{
    register int index;

    if ((index = ++(ray->Containing_Index)) >= MAX_CONTAINING_OBJECTS) {
        fprintf(stderr, "Too many nested refracting objects\n");
        closeAll();
        exit(1);
    }

    ray->Containing_Textures[index] = texture;
    ray->Containing_IORs[index] = texture->Object_Index_Of_Refraction;
}

void
Ray::exitContainingMedium()
{
    if (--(this->Containing_Index) < -1) {
        fprintf(stderr, "Too many exits from refractions\n");
        closeAll();
        exit(1);
    }
}
