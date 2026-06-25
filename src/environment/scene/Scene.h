#ifndef __SCENE__
#define __SCENE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"
#include "environment/light/Light.h"
#include "environment/material/Material.h"
#include "environment/geometry/BoundedGeometry.h"

class Scene {
  public:
    static constexpr double DEFAULT_ANTIALIAS_THRESHOLD = 0.3;

    Scene();
    ~Scene();

    const CameraSnapshot& getViewPoint() const { return viewPoint; }
    void setViewPoint(const CameraSnapshot &camera) { viewPoint = camera; }
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

    // The scene's default texture is aliased (not cloned) into every untextured
    // object's BoundedGeometry::objectTexture for the entire render (see
    // ~BoundedGeometry()'s PovRayMaterialConstancy guard) - it can only be freed
    // once nothing will read it again, i.e. once Scene itself is destroyed.
    // SceneParser::parse() calls this once, after parsing finishes, with
    // whatever the final default texture turned out to be (it may have been
    // replaced mid-parse by a `default { texture {...} }` block).
    void captureDefaultTexture(Material *texture) { defaultTexture = texture; }

  private:
    static ColorRgba blackFogColor();

    CameraSnapshot viewPoint;
    int screenHeight;
    int screenWidth;
    Light *lightSources;
    java::ArrayList<BoundedGeometry*> Objects{4};
    double atmosphereIor;
    double antialiasThreshold;
    double fogDistance;
    ColorRgba fogColor;
    Material *defaultTexture = nullptr;
};

#endif
