#ifndef __COLOUR_H__
#define __COLOUR_H__

#ifndef DBL
#define DBL double
#endif

#include "common/RGBAColor.h"
#include "common/RGBAColorPalette.h"
#include "common/RGBAColorPaletteSpan.h"

#define makeColour(c, r, g, b)                                                \
    {                                                                          \
        (c)->Red = (r);                                                        \
        (c)->Green = (g);                                                      \
        (c)->Blue = (b);                                                       \
        (c)->Alpha = 0.0;                                                      \
    }

extern DBL colourDistance(RGBAColor *colour1, RGBAColor *colour2);
extern void addColour(
    RGBAColor *result, RGBAColor *colour1, RGBAColor *colour2);
extern void scaleColour(RGBAColor *result, RGBAColor *colour, DBL factor);
extern void clipColour(RGBAColor *result, RGBAColor *colour);

#endif
