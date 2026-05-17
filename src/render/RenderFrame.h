#ifndef __RENDER_FRAME_H__
#define __RENDER_FRAME_H__

#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/light/Light.h"
#include "environment/camera/Viewpoint.h"

class RayWithSegments;

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
    static void createRay(RayWithSegments *ray, int width, int height, double x, double y);
    static void checkStats(int y);
    static void doAntiAliasing(int x, int y, RGBAColor *color);
    static void outputLine(int y);

    friend class RenderEngine;
};

#endif
