/****************************************************************************
 *                     ray.c
 *
 *  This module implements the code pertaining to rays.
 *
 *****************************************************************************/

#include "common/Ray.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "common/linealAlgebra/Vector3Dd.h"

inline void
Ray::mixVectorTerms(Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
{
    a.x = b.x * c.y;
    a.y = b.x * c.z;
    a.z = b.y * c.z;
}
void
Ray::makeRay()
{
    Vector3Dd tempInitDir;

    VectorOps::vSquareTerms(this->Initial_2, this->Initial);
    VectorOps::vSquareTerms(this->Direction_2, this->Direction);
    VectorOps::vEvaluate(this->Initial_Direction, this->Initial, this->Direction);
    Ray::mixVectorTerms(this->Mixed_Initial_Initial, this->Initial, this->Initial);
    Ray::mixVectorTerms(this->Mixed_Dir_Dir, this->Direction, this->Direction);
    Ray::mixVectorTerms(tempInitDir, this->Initial, this->Direction);
    Ray::mixVectorTerms(this->Mixed_Init_Dir, this->Direction, this->Initial);
    this->Mixed_Init_Dir.add(tempInitDir);
    this->Quadric_Constants_Cached = TRUE;
}

void
Ray::initializeContainers()
{
    this->Containing_Index = -1;
}

void
Ray::copyContainersFrom(Ray *sourceRay)
{
    register int i;

    if ((this->Containing_Index = sourceRay->Containing_Index) >=
        MAX_CONTAINING_OBJECTS) {
        fprintf(stderr, "ERROR - Containing Index too high\n");
        PovApp::closeAll();
        exit(1);
    }

    for (i = 0; i < MAX_CONTAINING_OBJECTS; i++) {
        this->Containing_Textures[i] = sourceRay->Containing_Textures[i];
        this->Containing_IORs[i] = sourceRay->Containing_IORs[i];
    }
}

void
Ray::enterContainingMedium(Texture *texture)
{
    register int index;

    if ((index = ++(this->Containing_Index)) >= MAX_CONTAINING_OBJECTS) {
        fprintf(stderr, "Too many nested refracting objects\n");
        PovApp::closeAll();
        exit(1);
    }

    this->Containing_Textures[index] = texture;
    this->Containing_IORs[index] = texture->Object_Index_Of_Refraction;
}

void
Ray::exitContainingMedium()
{
    if (--(this->Containing_Index) < -1) {
        fprintf(stderr, "Too many exits from refractions\n");
        PovApp::closeAll();
        exit(1);
    }
}
