#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/geometry/GeometryOperations.h"

#include "environment/geometry/volume/compound/Composite.h"

#include "environment/material/MaterialUtils.h"

#include "environment/scene/BoundedGeometryFactory.h"

#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

BoundedGeometry *
BoundedGeometryFactory::getObject()
{
    BoundedGeometry *newObject;

    if ((newObject = new BoundedGeometry()) == nullptr) {
        Logger::reportMessage("BoundedGeometryFactory", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }

    newObject->geometry = nullptr;
    newObject->objectTexture = MaterialUtils::instance().defaultTexture();
    newObject->objectColor = nullptr;
    newObject->noShadowFlag = false;
    return (newObject);
}
