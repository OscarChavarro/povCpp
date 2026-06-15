#include "environment/scene/SimpleBodyFactory.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/material/MaterialUtils.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/common/logging/Logger.h"

void
SimpleBodyFactory::link(
    SimpleBody *newObject, SimpleBody **field, SimpleBody **oldObjectList)
{
    *field = *oldObjectList;
    *oldObjectList = newObject;
}

SimpleBody *
SimpleBodyFactory::getObject()
{
    SimpleBody *newObject;

    if ((newObject = new SimpleBody()) == nullptr) {
        Logger::reportMessage("SimpleBodyFactory", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }

    newObject->nextObject = nullptr;
    newObject->geometry = nullptr;
    newObject->boundingShapes = nullptr;
    newObject->clippingShapes = nullptr;
    newObject->objectTexture = MaterialUtils::instance().defaultTexture();
    newObject->objectColor = nullptr;
    newObject->noShadowFlag = false;
    newObject->geometryType = GeometryTypes::OBJECT_TYPE;
    return (newObject);
}
#include "java/util/PriorityQueue.txx"
