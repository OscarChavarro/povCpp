#ifndef __COLOUR_H__
#define __COLOUR_H__

using DBL = double;

#include "common/RGBAColor.h"
#include "common/RGBAColorPalette.h"
#include "common/RGBAColorPaletteSpan.h"

template <typename ColourT, typename ScalarT>
inline void makeColour(ColourT *c, ScalarT r, ScalarT g, ScalarT b)
{
    c->Red = r;
    c->Green = g;
    c->Blue = b;
    c->Alpha = 0;
}

extern DBL colourDistance(RGBAColor *colour1, RGBAColor *colour2);
extern void addColour(
    RGBAColor *result, RGBAColor *colour1, RGBAColor *colour2);
extern void scaleColour(RGBAColor *result, RGBAColor *colour, DBL factor);
extern void clipColour(RGBAColor *result, RGBAColor *colour);

#endif
