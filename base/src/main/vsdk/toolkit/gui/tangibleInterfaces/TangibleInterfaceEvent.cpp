#include <cstdio>

#include "vsdk/toolkit/gui/tangibleInterfaces/TangibleInterfaceEvent.h"

TangibleInterfaceEvent::TangibleInterfaceEvent()
    : id(""), position(0, 0, 0), rotation()
{
}

TangibleInterfaceEvent::TangibleInterfaceEvent(
    const java::String& id, const Vector3Dd& position, const Quaterniond& rotation)
    : id(id), position(position), rotation(rotation)
{
}

const java::String& TangibleInterfaceEvent::getId() const
{
    return id;
}

const Vector3Dd& TangibleInterfaceEvent::getPosition() const
{
    return position;
}

const Quaterniond& TangibleInterfaceEvent::getRotation() const
{
    return rotation;
}

java::String TangibleInterfaceEvent::toString() const
{
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer),
        "TangibleInterfaceEvent{id=%s, position=(%.6f, %.6f, %.6f), rotation=(%.6f, %.6f, %.6f, %.6f)}",
        id.c_str(), position.x(), position.y(), position.z(),
        rotation.magnitude(), rotation.direction().x(), rotation.direction().y(), rotation.direction().z());
    return java::String(buffer);
}
