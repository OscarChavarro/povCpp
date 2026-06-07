/****************************************************************************
 *                     ray.c
 *
 *  This module implements the code pertaining to rays.
 *
 *****************************************************************************/

#include <cstdlib>

#include "environment/geometry/elements/RayWithSegments.h"
#include "common/logger/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "common/linealAlgebra/Vector3DdOps.h"

RayWithSegments::RayWithSegments()
{
    containingIndex = -1;
    quadricConstantsCached = false;
    isShadowRay = false;
    isPrimaryRay = false;
}

inline void
RayWithSegments::mixVectorTerms(
    Vector3Dd &a, const Vector3Dd &b, const Vector3Dd &c)
{
    a = Vector3Dd(b.x() * c.y(), b.x() * c.z(), b.y() * c.z());
}

void
RayWithSegments::makeRay()
{
    Vector3Dd tempInitDir;

    this->position2 = Vec3::squareTerms(this->position);
    this->direction2 = Vec3::squareTerms(this->direction);
    this->positionDirection = Vec3::evaluated(this->position, this->direction);
    RayWithSegments::mixVectorTerms(
        this->mixedPositionPosition, this->position, this->position);
    RayWithSegments::mixVectorTerms(
        this->mixedDirectionDirection, this->direction, this->direction);
    RayWithSegments::mixVectorTerms(
        tempInitDir, this->position, this->direction);
    RayWithSegments::mixVectorTerms(
        this->mixedPositionDirection, this->direction, this->position);
    this->mixedPositionDirection =
        this->mixedPositionDirection.add(tempInitDir);
    this->quadricConstantsCached = true;
}

void
RayWithSegments::initializeContainers()
{
    this->containingIndex = -1;
    this->isShadowRay = false;
    this->isPrimaryRay = false;
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
    this->isPrimaryRay = false;

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
