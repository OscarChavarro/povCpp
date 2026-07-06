#include "vsdk/toolkit/environment/light/Light.h"

Light::Light(const Vector3Dd &position, const ColorRgb &emission) :
    position(position),
    emission(emission),
    id(0),
    name("")
{
}

Light::Light(const Light &other) :
    position(other.position),
    emission(other.emission),
    id(other.id),
    name(other.name)
{
}

const java::String &
Light::getName() const
{
    return name;
}

void
Light::setName(const java::String &n)
{
    name = n;
}

int
Light::getId() const
{
    return id;
}

void
Light::setId(int i)
{
    id = i;
}

const Vector3Dd &
Light::getPosition() const
{
    return position;
}

void
Light::setPosition(const Vector3Dd &newPosition)
{
    position = Vector3Dd::copyOf(newPosition);
}

const ColorRgb &
Light::getEmission() const
{
    return emission;
}

void
Light::setEmission(const ColorRgb &newEmission)
{
    emission = ColorRgb(newEmission);
}

bool
Light::isAmbient() const
{
    return false;
}
