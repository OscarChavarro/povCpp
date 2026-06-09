/****************************************************************************
 *                         color.c
 *
 *  This module implements routines to manipulate colours.
 *
 *****************************************************************************/

#include "common/color/Color.h"

inline double
Color::Color::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

double
Color::colorDistance(ColorRgba *color1, ColorRgba *color2)
{
    return (Color::fabsInline(color1->getR() - color2->getR()) +
            Color::fabsInline(color1->getG() - color2->getG()) +
            Color::fabsInline(color1->getB() - color2->getB()));
}

void
Color::addColor(ColorRgba *result, ColorRgba *color1, ColorRgba *color2)
{
    result->setR(color1->getR() + color2->getR());
    result->setG(color1->getG() + color2->getG());
    result->setB(color1->getB() + color2->getB());
    result->setA(color1->getA() + color2->getA());
}

void
Color::scaleColor(ColorRgba *result, ColorRgba *color, double factor)
{
    result->setR(color->getR() * factor);
    result->setG(color->getG() * factor);
    result->setB(color->getB() * factor);
    result->setA(color->getA() * factor);
}

void
Color::clipColor(ColorRgba *result, ColorRgba *color)
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
