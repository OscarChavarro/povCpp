#ifndef __COLOUR_H__
#define __COLOUR_H__

class RGBAColor
{
  public:
    DBL Red, Green, Blue, Alpha;
};

class RGBAColorPaletteSpan
{
  public:
    DBL start, end;
    RGBAColor Start_Colour, End_Colour;
};

class RGBAColorPalette
{
  public:
    int Number_Of_Entries;
    RGBAColorPaletteSpan *Colour_Map_Entries;
    int Transparency_Flag;
};

#define Make_Colour(c,r,g,b) { (c)->Red=(r);(c)->Green=(g);(c)->Blue=(b); (c)->Alpha=0.0; }

extern DBL Colour_Distance(RGBAColor *colour1, RGBAColor *colour2);
extern void Add_Colour(RGBAColor *result, RGBAColor *colour1, RGBAColor *colour2);
extern void Scale_Colour(RGBAColor *result, RGBAColor *colour, DBL factor);
extern void Clip_Colour(RGBAColor *result, RGBAColor *colour);

#endif
