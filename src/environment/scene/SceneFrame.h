#ifndef __SCENE_FRAME_H__
#define __SCENE_FRAME_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "environment/light/Light.h"
#include "environment/scene/SceneObject.h"

class RenderFrame {
  public:
    Camera viewPoint;
    int screenHeight;
    int screenWidth;
    Light *lightSources;
    java::ArrayList<SceneObject*> Objects{4};
    double atmosphereIor;
    double antialiasThreshold;
    double fogDistance;
    ColorRgba fogColor;

    static void createRay(
        RayWithSegments *ray, int width, int height, double x, double y);
    static void checkStats(int y);
    static void doAntiAliasing(int x, int y, ColorRgba *color);
    static void outputLine(int y);
};

#endif
