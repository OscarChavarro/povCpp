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
    double Atmosphere_IOR, Antialias_Threshold;
    double Fog_Distance;
    RGBAColor Fog_Colour;

  private:
    static void createRay(Ray *ray, int width, int height, double x, double y);
    static void checkStats(int y);
    static void doAntiAliasing(int x, int y, RGBAColor *color);
    static void outputLine(int y);

    friend class RenderEngine;
};

class RenderEngine {
  public:
    static void readRenderedPart(void);
    static void supersample(
        RGBAColor *result, int x, int y, int width, int height);
    static void startTracing(void);
    static void trace(Ray *ray, RGBAColor *colour);
    static void initializeRenderer(void);
};

#endif
