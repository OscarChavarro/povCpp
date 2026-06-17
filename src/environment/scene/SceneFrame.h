#ifndef __SCENE_FRAME_H__
#define __SCENE_FRAME_H__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/camera/Camera.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/light/Light.h"
#include "environment/geometry/BoundedGeometry.h"

class RenderFrame {
  public:
    Camera viewPoint;
    int screenHeight;
    int screenWidth;
    Light *lightSources;
    java::ArrayList<BoundedGeometry*> Objects{4};
    double atmosphereIor;
    double antialiasThreshold;
    double fogDistance;
    ColorRgba fogColor;

    Camera& getViewPoint() { return viewPoint; }
    const Camera& getViewPoint() const { return viewPoint; }
    int& getScreenHeight() { return screenHeight; }
    int getScreenHeight() const { return screenHeight; }
    void setScreenHeight(int h) { screenHeight = h; }
    int& getScreenWidth() { return screenWidth; }
    int getScreenWidth() const { return screenWidth; }
    void setScreenWidth(int w) { screenWidth = w; }
    Light* getLightSources() const { return lightSources; }
    void setLightSources(Light* lights) { lightSources = lights; }
    double getAtmosphereIor() const { return atmosphereIor; }
    void setAtmosphereIor(double ior) { atmosphereIor = ior; }
    double getAntialiasThreshold() const { return antialiasThreshold; }
    void setAntialiasThreshold(double threshold) { antialiasThreshold = threshold; }
    double getFogDistance() const { return fogDistance; }
    void setFogDistance(double distance) { fogDistance = distance; }
    ColorRgba& getFogColor() { return fogColor; }
    const ColorRgba& getFogColor() const { return fogColor; }
    java::ArrayList<BoundedGeometry*>& getObjects() { return Objects; }
    const java::ArrayList<BoundedGeometry*>& getObjects() const { return Objects; }

    static void createRay(
        RayWithSegments *ray, int width, int height, double x, double y);
    static void checkStats(int y);
    static void doAntiAliasing(int x, int y, ColorRgba *color);
    static void outputLine(int y);
};

#endif
