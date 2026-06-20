#include "vsdk/toolkit/common/logging/Logger.h"
#include "environment/material/ValuesBuilder.h"

ColorRgba *
ValuesBuilder::getColor()
{
    ColorRgba *newColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
    if (newColor == nullptr) {
        Logger::reportMessage("ValuesBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate color\n");
    }
    return (newColor);
}

Vector3Dd *
ValuesBuilder::getVector()
{
    Vector3Dd *newVector = new Vector3Dd;
    if (newVector == nullptr) {
        Logger::reportMessage("ValuesBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate vector\n");
    }
    return (newVector);
}

double *
ValuesBuilder::getFloat()
{
    double *newFloat = new double(0.0);
    if (newFloat == nullptr) {
        Logger::reportMessage("ValuesBuilder", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate float\n");
    }
    return (newFloat);
}
