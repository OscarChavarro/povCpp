#ifndef __SCENE__
#define __SCENE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/camera/Camera.h"
#include "environment/light/Light.h"
#include "environment/geometry/BoundedGeometry.h"

class Scene {
  public:
    static constexpr double DEFAULT_ANTIALIAS_THRESHOLD = 0.3;

    Scene();
    // Objects is owned (see ~Scene()); a compiler-generated copy would only
    // shallow-copy the ArrayList, leaving two Scenes pointing at the same
    // BoundedGeometry* instances. Nothing in this codebase copies a Scene by
    // value (SceneParser::parse() used to and was changed to parse in place
    // instead) - deleted here to keep it that way.
    Scene(const Scene &other) = delete;
    Scene &operator=(const Scene &other) = delete;
    ~Scene();

    const Camera& getViewPoint() const { return viewPoint; }
    void setViewPoint(const Camera &camera) { viewPoint = camera; }
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
    void setFog(const ColorRgba& color, double distance)
    {
        fogColor = color;
        fogDistance = distance;
    }
    const ColorRgba& getFogColor() const { return fogColor; }
    const java::ArrayList<BoundedGeometry*>& getObjects() const { return Objects; }
    void setObjects(const java::ArrayList<BoundedGeometry*> &objects)
    {
        Objects = objects;
    }
    void resetForSceneParse(double antialiasThreshold = DEFAULT_ANTIALIAS_THRESHOLD);

  private:
    Camera viewPoint;
    int screenHeight;
    int screenWidth;
    Light *lightSources;
    java::ArrayList<BoundedGeometry*> Objects{4};
    double atmosphereIor;
    double antialiasThreshold;
    double fogDistance;
    ColorRgba fogColor;
};

#endif
