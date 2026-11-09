#include "vsdk/toolkit/environment/background/SimpleBackground.h"
SimpleBackground::SimpleBackground() : color(0, 0, 0)
{
}

ColorRgb SimpleBackground::colorInDireccion(const Vector3Dd&)
{
    return ColorRgb(color);
}

void SimpleBackground::setColor(double r, double g, double b)
{
    color = ColorRgb(r, g, b);
}
