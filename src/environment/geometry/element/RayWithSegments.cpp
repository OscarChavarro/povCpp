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
    requiredDetailMask = RayWithSegments::DETAIL_ALL;
    statistics = nullptr;
    config = nullptr;
    intersectionQueuePool = nullptr;
}

RayWithSegments::RayWithSegments(
    LocalIntersectionClone, const RayWithSegments &source) :
    // Ray's copy constructor copies origin/direction/t without renormalizing
    // (unlike Ray's value constructors, which call normalizeDirection - a sqrt
    // we must not pay here). The origin/direction are overwritten with the
    // local-space ray before use, but the plain copy is still cheaper than the
    // renormalizing path, so we copy the base wholesale.
    Ray(source),
    // The six quadric-cache vectors are deliberately left default rather than
    // copied: overwriting origin/direction forces quadricConstantsCached false,
    // so Quadric::doIntersectionForAllRayCrossings rebuilds them via makeRay()
    // on first use (the original full-copy path also reset the flag).
    containingIndex(-1),
    // Capacity 0: no backing allocation. The intersection path never enters a
    // containing medium, so these stay empty; see the header note above.
    containingTextures(0),
    containingIORs(0),
    quadricConstantsCached(false),
    isShadowRay(source.isShadowRay),
    isPrimaryRay(source.isPrimaryRay),
    requiredDetailMask(source.requiredDetailMask),
    statistics(source.statistics),
    config(source.config),
    intersectionQueuePool(source.intersectionQueuePool)
{
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
    this->requiredDetailMask = RayWithSegments::DETAIL_ALL;
}

void
RayWithSegments::copyContainersFrom(const RayWithSegments *sourceRay)
{
    this->containingIndex = sourceRay->containingIndex;
    this->isShadowRay = sourceRay->isShadowRay;
    this->isPrimaryRay = false;
    this->requiredDetailMask = sourceRay->requiredDetailMask;
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
