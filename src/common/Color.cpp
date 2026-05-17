/****************************************************************************
 *                         colour.c
 *
 *  This module implements routines to manipulate colours.
 *
 *****************************************************************************/

#include "common/Frame.h"
#include "app/PovApp.h"
#include "common/Vector.h"

inline double
Color::Color::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

double
Color::colorDistance(RGBAColor *color1, RGBAColor *color2)
{
    return (Color::fabsInline(color1->Red - color2->Red) +
            Color::fabsInline(color1->Green - color2->Green) +
            Color::fabsInline(color1->Blue - color2->Blue));
}

void
Color::addColor(RGBAColor *result, RGBAColor *color1, RGBAColor *color2)
{
    result->Red = color1->Red + color2->Red;
    result->Green = color1->Green + color2->Green;
    result->Blue = color1->Blue + color2->Blue;
    result->Alpha = color1->Alpha + color2->Alpha;
}

void
Color::scaleColor(RGBAColor *result, RGBAColor *color, double factor)
{
    result->Red = color->Red * factor;
    result->Green = color->Green * factor;
    result->Blue = color->Blue * factor;
    result->Alpha = color->Alpha * factor;
}

void
Color::clipColor(RGBAColor *result, RGBAColor *color)
{
    if (color->Red > 1.0) {
        result->Red = 1.0;
    } else if (color->Red < 0.0) {
        result->Red = 0.0;
    } else {
        result->Red = color->Red;
    }

    if (color->Green > 1.0) {
        result->Green = 1.0;
    } else if (color->Green < 0.0) {
        result->Green = 0.0;
    } else {
        result->Green = color->Green;
    }

    if (color->Blue > 1.0) {
        result->Blue = 1.0;
    } else if (color->Blue < 0.0) {
        result->Blue = 0.0;
    } else {
        result->Blue = color->Blue;
    }

    if (color->Alpha > 1.0) {
        result->Alpha = 1.0;
    } else if (color->Alpha < 0.0) {
        result->Alpha = 0.0;
    } else {
        result->Alpha = color->Alpha;
    }
}
