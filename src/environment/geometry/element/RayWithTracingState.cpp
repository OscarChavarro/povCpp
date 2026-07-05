/**
This module implements the code pertaining to rays.
*/

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/material/Material.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "java/util/ArrayList.txx"

template class java::ArrayList<Material *>;

RayWithTracingState::RayWithTracingState() :
    containingTextures(RayWithTracingState::MAX_CONTAINING_OBJECTS),
    containingIORs(RayWithTracingState::MAX_CONTAINING_OBJECTS)
{
    containingIndex = -1;
    quadricConstantsCached = false;
    aabbReciprocalsCached = false;
    isShadowRay = false;
    isPrimaryRay = false;
    requiredDetailMask = RayWithTracingState::DETAIL_ALL;
    statistics = nullptr;
    config = nullptr;
    intersectionQueuePool = nullptr;
}

void
RayWithTracingState::initializeContainers()
{
    this->containingIndex = -1;
    this->isShadowRay = false;
    this->isPrimaryRay = false;
    this->requiredDetailMask = RayWithTracingState::DETAIL_ALL;
}

void
RayWithTracingState::copyContainersFrom(const RayWithTracingState *sourceRay)
{
    this->containingIndex = sourceRay->containingIndex;
    this->isShadowRay = sourceRay->isShadowRay;
    this->isPrimaryRay = false;
    this->requiredDetailMask = sourceRay->requiredDetailMask;
    this->containingTextures = sourceRay->containingTextures;
    this->containingIORs = sourceRay->containingIORs;
}

void
RayWithTracingState::enterContainingMedium(Material *texture, double indexOfRefraction)
{
    int index;

    index = ++(this->containingIndex);

    if (index < this->containingTextures.size()) {
        this->containingTextures.set(index, texture);
        this->containingIORs.set(index, indexOfRefraction);
    } else {
        this->containingTextures.add(texture);
        this->containingIORs.add(indexOfRefraction);
    }
}

void
RayWithTracingState::exitContainingMedium()
{
    if (--(this->containingIndex) < -1) {
        Logger::reportMessage("RayWithTracingState", Logger::FATAL_ERROR, "", "Too many exits from refractions\n");
    }
}
