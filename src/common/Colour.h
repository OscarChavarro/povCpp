#ifndef __COLOUR_H__
#define __COLOUR_H__

#ifndef DBL
#define DBL double
#endif

#include "common/RGBAColor.h"
#include "common/RGBAColorPaletteSpan.h"
#include "common/RGBAColorPalette.h"

#define Make_Colour(c, r, g, b)                                                \
    {                                                                          \
        (c)->Red = (r);                                                        \
        (c)->Green = (g);                                                      \
        (c)->Blue = (b);                                                       \
        (c)->Alpha = 0.0;                                                      \
    }

extern DBL Colour_Distance(RGBAColor *colour1, RGBAColor *colour2);
extern void Add_Colour(
    RGBAColor *result, RGBAColor *colour1, RGBAColor *colour2);
extern void Scale_Colour(RGBAColor *result, RGBAColor *colour, DBL factor);
extern void Clip_Colour(RGBAColor *result, RGBAColor *colour);

#endif
