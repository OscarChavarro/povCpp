/**
This module implements the code pertaining to rays.
*/

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "java/util/ArrayList.txx"

template class java::ArrayList<Material *>;

RayWithSegments::RayWithSegments() :
    containingTextures(RayWithSegments::MAX_CONTAINING_OBJECTS),
    containingIORs(RayWithSegments::MAX_CONTAINING_OBJECTS)
{
    containingIndex = -1;
    quadricConstantsCached = false;
    isShadowRay = false;
    isPrimaryRay = false;
    statistics = nullptr;
    config = nullptr;
    intersectionQueuePool = nullptr;
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
    const Vector3Dd& origin = getOrigin();
    const Vector3Dd& direction = getDirection();

    this->position2 = origin.multiply(origin);
    this->direction2 = direction.multiply(direction);
    this->positionDirection = origin.multiply(direction);
    RayWithSegments::mixVectorTerms(
        this->mixedPositionPosition, origin, origin);
    RayWithSegments::mixVectorTerms(
        this->mixedDirectionDirection, direction, direction);
    RayWithSegments::mixVectorTerms(
        tempInitDir, origin, direction);
    RayWithSegments::mixVectorTerms(
        this->mixedPositionDirection, direction, origin);
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
RayWithSegments::copyContainersFrom(const RayWithSegments *sourceRay)
{
    this->containingIndex = sourceRay->containingIndex;
    this->isShadowRay = sourceRay->isShadowRay;
    this->isPrimaryRay = false;
    this->containingTextures = sourceRay->containingTextures;
    this->containingIORs = sourceRay->containingIORs;
}

void
RayWithSegments::enterContainingMedium(Material *texture)
{
    int index;

    index = ++(this->containingIndex);

    if (index < this->containingTextures.size()) {
        this->containingTextures.set(index, texture);
        this->containingIORs.set(index, texture->getObjectIndexOfRefraction());
    } else {
        this->containingTextures.add(texture);
        this->containingIORs.add(texture->getObjectIndexOfRefraction());
    }
}

void
RayWithSegments::exitContainingMedium()
{
    if (--(this->containingIndex) < -1) {
        Logger::reportMessage("RayWithSegments", Logger::FATAL_ERROR, "", "Too many exits from refractions\n");
    }
}
