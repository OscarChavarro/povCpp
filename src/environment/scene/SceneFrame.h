#ifndef __SCENE_FRAME_H__
#define __SCENE_FRAME_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "environment/light/Light.h"
#include "environment/scene/SceneObject.h"

class RayWithSegments;

class RenderFrame {
  public:
    Camera viewPoint;
    int screenHeight;
    int screenWidth;
    Light *Light_Sources;
    SceneObject *Objects;
    double atmosphereIor;
    double antialiasThreshold;
    double fogDistance;
    RGBAColor fogColour;

  private:
    static void createRay(
        RayWithSegments *ray, int width, int height, double x, double y);
    static void checkStats(int y);
    static void doAntiAliasing(int x, int y, RGBAColor *color);
    static void outputLine(int y);

    friend class RenderEngine;
};

#endif
