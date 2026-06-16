#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/geometry/GeometryOperations.h"

#include "environment/geometry/volume/compound/Composite.h"

#include "environment/material/MaterialUtils.h"

#include "environment/scene/SimpleBodyFactory.h"

#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

SimpleBody *
SimpleBodyFactory::getObject()
{
    SimpleBody *newObject;

    if ((newObject = new SimpleBody()) == nullptr) {
        Logger::reportMessage("SimpleBodyFactory", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }

    newObject->geometry = nullptr;
    newObject->objectTexture = MaterialUtils::instance().defaultTexture();
    newObject->objectColor = nullptr;
    newObject->noShadowFlag = false;
    return (newObject);
}
