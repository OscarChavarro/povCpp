/****************************************************************************
 *                         colour.c
 *
 *  This module implements routines to manipulate colours.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"

/*===========================================================================*/

inline DBL fabsInline(DBL x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

/*===========================================================================*/

DBL
colourDistance(RGBAColor *colour1, RGBAColor *colour2)
{
    return (fabsInline(colour1->Red - colour2->Red) +
            fabsInline(colour1->Green - colour2->Green) +
            fabsInline(colour1->Blue - colour2->Blue));
}

void
addColour(RGBAColor *result, RGBAColor *colour1, RGBAColor *colour2)
{
    result->Red = colour1->Red + colour2->Red;
    result->Green = colour1->Green + colour2->Green;
    result->Blue = colour1->Blue + colour2->Blue;
    result->Alpha = colour1->Alpha + colour2->Alpha;
}

void
scaleColour(RGBAColor *result, RGBAColor *colour, DBL factor)
{
    result->Red = colour->Red * factor;
    result->Green = colour->Green * factor;
    result->Blue = colour->Blue * factor;
    result->Alpha = colour->Alpha * factor;
}

void
clipColour(RGBAColor *result, RGBAColor *colour)
{
    if (colour->Red > 1.0) {
        result->Red = 1.0;
    } else if (colour->Red < 0.0) {
        result->Red = 0.0;
    } else {
        result->Red = colour->Red;
    }

    if (colour->Green > 1.0) {
        result->Green = 1.0;
    } else if (colour->Green < 0.0) {
        result->Green = 0.0;
    } else {
        result->Green = colour->Green;
    }

    if (colour->Blue > 1.0) {
        result->Blue = 1.0;
    } else if (colour->Blue < 0.0) {
        result->Blue = 0.0;
    } else {
        result->Blue = colour->Blue;
    }

    if (colour->Alpha > 1.0) {
        result->Alpha = 1.0;
    } else if (colour->Alpha < 0.0) {
        result->Alpha = 0.0;
    } else {
        result->Alpha = colour->Alpha;
    }
}
