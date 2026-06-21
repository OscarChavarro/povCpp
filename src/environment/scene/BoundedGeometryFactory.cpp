#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "environment/scene/BoundedGeometryFactory.h"

#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

BoundedGeometry *
BoundedGeometryFactory::getBoundedGeometry(PovRayMaterial *defaultTexture)
{
    return getBoundedGeometry(nullptr, defaultTexture);
}

BoundedGeometry *
BoundedGeometryFactory::getBoundedGeometry(TransformableElement *geometry, PovRayMaterial *defaultTexture)
{
    BoundedGeometry *newObject = new BoundedGeometry(
        geometry,
        defaultTexture,
        nullptr,
        false);
    if (newObject == nullptr) {
        Logger::reportMessage("BoundedGeometryFactory", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }
    return (newObject);
}
