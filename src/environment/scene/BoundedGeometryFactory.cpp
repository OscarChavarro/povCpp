#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/common/logging/Logger.h"


#include "environment/geometry/volume/compound/Composite.h"

#include "environment/material/MaterialUtils.h"

#include "environment/scene/BoundedGeometryFactory.h"

#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

BoundedGeometry *
BoundedGeometryFactory::getObject()
{
    return getObject(nullptr);
}

BoundedGeometry *
BoundedGeometryFactory::getObject(TransformableElement *geometry)
{
    BoundedGeometry *newObject = new BoundedGeometry(
        geometry,
        MaterialUtils::instance().defaultTexture(),
        nullptr,
        false);
    if (newObject == nullptr) {
        Logger::reportMessage("BoundedGeometryFactory", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }
    return (newObject);
}
