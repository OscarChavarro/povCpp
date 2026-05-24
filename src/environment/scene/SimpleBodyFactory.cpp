#include "environment/scene/SimpleBodyFactory.h"

#include "common/logger/Logger.h"
#include "environment/geometry/GeometryOperations.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "media/TextureUtils.h"

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
        Logger::error("Out of memory. Cannot allocate object");
        exit(1);
    }

    newObject->nextObject = nullptr;
    newObject->Shape = nullptr;
    newObject->boundingShapes = nullptr;
    newObject->clippingShapes = nullptr;
    newObject->objectTexture = TextureUtils::defaultTexture();
    newObject->objectColour = nullptr;
    newObject->noShadowFlag = LegacyBoolean::FALSE_VALUE;
    newObject->Type = GeometryOperations::OBJECT_TYPE;
    newObject->methods = &Composite::basicObjectMethodTable;
    return (newObject);
}
