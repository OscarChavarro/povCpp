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
    aabbReciprocalsCached = false;
    isShadowRay = false;
    isPrimaryRay = false;
    requiredDetailMask = RayWithSegments::DETAIL_ALL;
    statistics = nullptr;
    config = nullptr;
    intersectionQueuePool = nullptr;
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
