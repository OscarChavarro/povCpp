#include "vsdk/toolkit/common/logging/Logger.h"

#include "io/pov/geometry/SceneBuilder.h"

SimpleBodyBuilder *
SceneBuilder::wrap(TransformedGeometry *geometry)
{
    SimpleBodyBuilder *body = new SimpleBodyBuilder(geometry, nullptr, nullptr);
    if (body == nullptr) {
        Logger::reportMessage("SceneBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate body\n");
    }
    return (body);
}
