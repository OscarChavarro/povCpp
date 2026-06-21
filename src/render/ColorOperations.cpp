#include "render/ColorOperations.h"

inline double
ColorOperations::absoluteValue(double x)
{
    // Note this is not equivalent to Math.abs
    return (x < 0.0) ? (0.0 - x) : x;
}

double
ColorOperations::colorDistance(const ColorRgba *color1, const ColorRgba *color2)
{
    return (ColorOperations::absoluteValue(color1->getR() - color2->getR()) +
            ColorOperations::absoluteValue(color1->getG() - color2->getG()) +
            ColorOperations::absoluteValue(color1->getB() - color2->getB()));
}

void
ColorOperations::addColor(ColorRgba *result, const ColorRgba *color1, const ColorRgba *color2)
{
    result->setR(color1->getR() + color2->getR());
    result->setG(color1->getG() + color2->getG());
    result->setB(color1->getB() + color2->getB());
    result->setA(color1->getA() + color2->getA());
}

void
ColorOperations::scaleColor(ColorRgba *result, const ColorRgba *color, double factor)
{
    result->setR(color->getR() * factor);
    result->setG(color->getG() * factor);
    result->setB(color->getB() * factor);
    result->setA(color->getA() * factor);
}

void
ColorOperations::clipColor(ColorRgba *result, const ColorRgba *color)
{
    if (color->getR() > 1.0) {
        result->setR(1.0);
    } else if (color->getR() < 0.0) {
        result->setR(0.0);
    } else {
        result->setR(color->getR());
    }

    if (color->getG() > 1.0) {
        result->setG(1.0);
    } else if (color->getG() < 0.0) {
        result->setG(0.0);
    } else {
        result->setG(color->getG());
    }

    if (color->getB() > 1.0) {
        result->setB(1.0);
    } else if (color->getB() < 0.0) {
        result->setB(0.0);
    } else {
        result->setB(color->getB());
    }

    if (color->getA() > 1.0) {
        result->setA(1.0);
    } else if (color->getA() < 0.0) {
        result->setA(0.0);
    } else {
        result->setA(color->getA());
    }
}
