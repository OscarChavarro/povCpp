#ifndef __RENDER_H__
#define __RENDER_H__

#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "geom/Light.h"
#include "geom/ViewPnt.h"

class Frame {
  public:
    Viewpoint View_Point;
    int Screen_Height, Screen_Width;
    Light *Light_Sources;
    SimpleBody *Objects;
    DBL Atmosphere_IOR, Antialias_Threshold;
    DBL Fog_Distance;
    RGBAColor Fog_Colour;

  private:
    static void createRay(Ray *ray, int width, int height, DBL x, DBL y);
    static void checkStats(int y);
    static void doAntiAliasing(int x, int y, RGBAColor *color);
    static void outputLine(int y);

    friend void Supersample(RGBAColor *result, int x, int y, int Width, int Height);
    friend void startTracing(void);
};

extern void readRenderedPart(void);
extern void Supersample(RGBAColor *result, int x, int y, int Width, int Height);
extern void startTracing(void);
extern void Trace(Ray *ray, RGBAColor *Colour);
extern void initializeRenderer(void);

#endif
