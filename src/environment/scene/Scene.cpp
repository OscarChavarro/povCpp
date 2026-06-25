#include "java/util/ArrayList.txx"

#include "vsdk/toolkit/environment/camera/Camera.h"

#include "environment/scene/Scene.h"

namespace {
CameraSnapshot
defaultViewPoint()
{
    const Vector3Dd location(0.0, 0.0, 0.0);
    const Vector3Dd direction(0.0, 0.0, 1.0);
    const Vector3Dd up(0.0, 1.0, 0.0);
    const Vector3Dd right(1.33, 0.0, 0.0);
    const Vector3Dd front = direction.normalizedFast();
    const Vector3Dd left = right.normalizedFast().multiply(-1.0);
    const Vector3Dd upNormalized = up.normalizedFast();

    return CameraSnapshot(
        location,
        front,
        left,
        upNormalized,
        Camera::PROJECTION_MODE_PERSPECTIVE,
        1.0,
        0.0,
        0.0,
        direction,
        up,
        right);
}
}

ColorRgba
Scene::blackFogColor()
{
    return ColorRgba(0.0, 0.0, 0.0, 0.0);
}

Scene::Scene() :
    viewPoint(defaultViewPoint()),
    screenHeight(0),
    screenWidth(0),
    lightSources(nullptr),
    atmosphereIor(1.0),
    antialiasThreshold(Scene::DEFAULT_ANTIALIAS_THRESHOLD),
    fogDistance(0.0),
    fogColor(0.0, 0.0, 0.0, 0.0)
{
}

Scene::~Scene()
{
    // lightSources is not deleted here: it is a non-owning traversal pointer
    // into the same BoundedGeometry/LightGeometryAdapter tree already owned by
    // Objects (see ScenePostProcessor::linkLights), not a separate allocation.
    for (long int i = 0; i < Objects.size(); i++) {
        delete Objects[i];
    }
    delete defaultTexture;
}

void
Scene::resetForSceneParse(double antialiasThreshold)
{
    viewPoint = defaultViewPoint();
    lightSources = nullptr;
    Objects.clear();
    atmosphereIor = 1.0;
    this->antialiasThreshold = antialiasThreshold;
    setFog(blackFogColor(), 0.0);
}
