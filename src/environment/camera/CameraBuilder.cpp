#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/camera/Camera.h"
#include "environment/camera/CameraBuilder.h"

Camera *
CameraBuilder::getCamera()
{
    Camera *newViewpoint = new Camera();
    if (newViewpoint == nullptr) {
        Logger::reportMessage("CameraBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate viewpoint\n");
    }
    return (newViewpoint);
}
