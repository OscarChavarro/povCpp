#ifndef __SCENE__
#define __SCENE__

#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"
#include "vsdk/toolkit/environment/light/Light.h"
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"

class Scene : public SimpleScene {
  public:
    static constexpr double DEFAULT_ANTIALIAS_THRESHOLD = 0.3;

    Scene();
    ~Scene();

    SimpleSceneSnapshot* exportToSimpleSceneSnapshot() override {
        return SimpleScene::exportToSimpleSceneSnapshot(new CameraSnapshot(viewPoint), nullptr);
    }
    SimpleSceneSnapshot* exportToSimpleSceneSnapshot(int, int) override {
        return exportToSimpleSceneSnapshot();
    }

    const CameraSnapshot& getViewPoint() const { return viewPoint; }
    void setViewPoint(const CameraSnapshot &camera) { viewPoint = camera; }
    int& getScreenHeight() { return screenHeight; }
    int getScreenHeight() const { return screenHeight; }
    void setScreenHeight(int h) { screenHeight = h; }
    int& getScreenWidth() { return screenWidth; }
    int getScreenWidth() const { return screenWidth; }
    void setScreenWidth(int w) { screenWidth = w; }
    const java::ArrayList<Light*>& getLightSources() const { return getLights(); }
    java::ArrayList<Light*>& getLightSources() { return getLights(); }
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
    const java::ArrayList<SimpleBody*>& getObjects() const { return getSimpleBodies(); }
    void setObjects(const java::ArrayList<RayCastingBody*>& objects) {
        java::ArrayList<SimpleBody*> bodies;
        for (long int i = 0; i < objects.size(); ++i) bodies.add(objects[i]);
        setSimpleBodies(bodies);
    }

    // The scene's default texture is aliased (not cloned) into every untextured
    // object's RayCastingBody::objectTexture for the entire render (see
    // ~RayCastingBody()'s objectTextureReleaser guard) - it can only be freed
    // once nothing will read it again, i.e. once Scene itself is destroyed.
    // SceneParser::parse() calls this once, after parsing finishes, with
    // whatever the final default texture turned out to be (it may have been
    // replaced mid-parse by a `default { texture {...} }` block).
    void captureDefaultTexture(Material *texture) { defaultTexture = texture; }

    void resetForSceneParse(double antialiasThreshold = DEFAULT_ANTIALIAS_THRESHOLD);

  private:
    static ColorRgba blackFogColor();
    static CameraSnapshot defaultViewPoint();

    CameraSnapshot viewPoint;
    int screenHeight;
    int screenWidth;
    double atmosphereIor;
    double antialiasThreshold;
    double fogDistance;
    ColorRgba fogColor;
    Material *defaultTexture = nullptr;
};

#endif
