#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/scene/SceneBuilder.h"
#include "environment/scene/SimpleBody.h"

SimpleBody *
SceneBuilder::wrap(Geometry *geometry)
{
    SimpleBody *body = new SimpleBody(geometry, nullptr, nullptr);
    if (body == nullptr) {
        Logger::reportMessage("SceneBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate body\n");
    }
    return (body);
}
