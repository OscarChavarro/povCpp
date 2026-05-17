#ifndef __RENDER_FRAME_H__
#define __RENDER_FRAME_H__

#include "app/PovApp.h"
#include "common/Vector3D.h"
#include "geom/Light.h"
#include "geom/Viewpoint.h"

class RenderFrame {
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

#endif
