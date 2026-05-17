/****************************************************************************
 *                     ray.c
 *
 *  This module implements the code pertaining to rays.
 *
 *****************************************************************************/

#include "environment/geometry/elements/Ray.h"
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

    VectorOps::vSquareTerms(this->position2, this->position);
    VectorOps::vSquareTerms(this->direction2, this->direction);
    VectorOps::vEvaluate(this->positionDirection, this->position, this->direction);
    Ray::mixVectorTerms(this->mixedPositionPosition, this->position, this->position);
    Ray::mixVectorTerms(this->mixedDirectionDirection, this->direction, this->direction);
    Ray::mixVectorTerms(tempInitDir, this->position, this->direction);
    Ray::mixVectorTerms(this->mixedPositionDirection, this->direction, this->position);
    this->mixedPositionDirection.add(tempInitDir);
    this->quadricConstantsCached = TRUE;
}

void
Ray::initializeContainers()
{
    this->containingIndex = -1;
}

void
Ray::copyContainersFrom(Ray *sourceRay)
{
    register int i;

    if ((this->containingIndex = sourceRay->containingIndex) >=
        MAX_CONTAINING_OBJECTS) {
        fprintf(stderr, "ERROR - Containing Index too high\n");
        PovApp::closeAll();
        exit(1);
    }

    for (i = 0; i < MAX_CONTAINING_OBJECTS; i++) {
        this->containingTextures[i] = sourceRay->containingTextures[i];
        this->containingIORs[i] = sourceRay->containingIORs[i];
    }
}

void
Ray::enterContainingMedium(Texture *texture)
{
    register int index;

    if ((index = ++(this->containingIndex)) >= MAX_CONTAINING_OBJECTS) {
        fprintf(stderr, "Too many nested refracting objects\n");
        PovApp::closeAll();
        exit(1);
    }

    this->containingTextures[index] = texture;
    this->containingIORs[index] = texture->Object_Index_Of_Refraction;
}

void
Ray::exitContainingMedium()
{
    if (--(this->containingIndex) < -1) {
        fprintf(stderr, "Too many exits from refractions\n");
        PovApp::closeAll();
        exit(1);
    }
}
