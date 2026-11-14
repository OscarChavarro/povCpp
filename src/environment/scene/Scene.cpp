
#include "vsdk/toolkit/environment/camera/Camera.h"

#include "environment/scene/Scene.h"
#include "java/util/ArrayList.txx"

ColorRgba
Scene::blackFogColor()
{
    return ColorRgba(0.0, 0.0, 0.0, 0.0);
}

CameraSnapshot
Scene::defaultViewPoint()
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

Scene::Scene() :
    viewPoint(defaultViewPoint()),
    screenHeight(0),
    screenWidth(0),
    atmosphereIor(1.0),
    antialiasThreshold(Scene::DEFAULT_ANTIALIAS_THRESHOLD),
    fogDistance(0.0),
    fogColor(0.0, 0.0, 0.0, 0.0)
{
}

Scene::~Scene()
{
    deleteObjects();
    clearOwnedElements();
    delete defaultTexture;
}

void
Scene::resetForSceneParse(double antialiasThreshold)
{
    viewPoint = defaultViewPoint();
    deleteObjects();
    clearOwnedElements();
    delete defaultTexture;
    defaultTexture = nullptr;
    atmosphereIor = 1.0;
    this->antialiasThreshold = antialiasThreshold;
    setFog(blackFogColor(), 0.0);
}

void
Scene::setObjects(const java::ArrayList<PovrayBody*>& newObjects)
{
    deleteObjects();
    for (long int i = 0; i < newObjects.size(); i++) objects.add(newObjects.get(i));
}

void
Scene::deleteObjects()
{
    for (long int i = 0; i < objects.size(); ++i) {
        delete objects[i];
    }
    objects.clear();
}
