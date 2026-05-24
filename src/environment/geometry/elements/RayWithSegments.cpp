/****************************************************************************
 *                     ray.c
 *
 *  This module implements the code pertaining to rays.
 *
 *****************************************************************************/

#include <cstdlib>

#include "environment/geometry/elements/RayWithSegments.h"
#include "common/logger/Logger.h"
#include "common/linealAlgebra/Vector3Dd.h"

RayWithSegments::RayWithSegments()
{
    containingIndex = -1;
    quadricConstantsCached = LegacyBoolean::FALSE_VALUE;
    isShadowRay = LegacyBoolean::FALSE_VALUE;
    isPrimaryRay = LegacyBoolean::FALSE_VALUE;
}

inline void
RayWithSegments::mixVectorTerms(
    Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
{
    a.x = b.x * c.y;
    a.y = b.x * c.z;
    a.z = b.y * c.z;
}

void
RayWithSegments::makeRay()
{
    Vector3Dd tempInitDir;

    VectorOps::vSquareTerms(this->position2, this->position);
    VectorOps::vSquareTerms(this->direction2, this->direction);
    VectorOps::vEvaluate(
        this->positionDirection, this->position, this->direction);
    RayWithSegments::mixVectorTerms(
        this->mixedPositionPosition, this->position, this->position);
    RayWithSegments::mixVectorTerms(
        this->mixedDirectionDirection, this->direction, this->direction);
    RayWithSegments::mixVectorTerms(
        tempInitDir, this->position, this->direction);
    RayWithSegments::mixVectorTerms(
        this->mixedPositionDirection, this->direction, this->position);
    this->mixedPositionDirection.add(tempInitDir);
    this->quadricConstantsCached = LegacyBoolean::TRUE_VALUE;
}

void
RayWithSegments::initializeContainers()
{
    this->containingIndex = -1;
    this->isShadowRay = LegacyBoolean::FALSE_VALUE;
    this->isPrimaryRay = LegacyBoolean::FALSE_VALUE;
}

void
RayWithSegments::copyContainersFrom(RayWithSegments *sourceRay)
{
    int i;

    if ((this->containingIndex = sourceRay->containingIndex) >=
        RayWithSegments::MAX_CONTAINING_OBJECTS) {
        Logger::error( "ERROR - Containing Index too high\n");
        exit(1);
    }
    this->isShadowRay = sourceRay->isShadowRay;
    this->isPrimaryRay = LegacyBoolean::FALSE_VALUE;

    for (i = 0; i < RayWithSegments::MAX_CONTAINING_OBJECTS; i++) {
        this->containingTextures[i] = sourceRay->containingTextures[i];
        this->containingIORs[i] = sourceRay->containingIORs[i];
    }
}

void
RayWithSegments::enterContainingMedium(Texture *texture)
{
    int index;

    if ((index = ++(this->containingIndex)) >= RayWithSegments::MAX_CONTAINING_OBJECTS) {
        Logger::error( "Too many nested refracting objects\n");
        exit(1);
    }

    this->containingTextures[index] = texture;
    this->containingIORs[index] = texture->objectIndexOfRefraction;
}

void
RayWithSegments::exitContainingMedium()
{
    if (--(this->containingIndex) < -1) {
        Logger::error( "Too many exits from refractions\n");
        exit(1);
    }
}
