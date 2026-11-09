#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/background/Background.h"
#include "vsdk/toolkit/environment/camera/Camera.h"
#include "vsdk/toolkit/environment/camera/CameraSnapshot.h"
#include "vsdk/toolkit/environment/light/Light.h"
#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/SimpleScene.h"
#include "vsdk/toolkit/environment/scene/SimpleSceneSnapshot.h"
SimpleScene::SimpleScene()
    : activeCameraIndex(0), activeBackgroundIndex(0)
{
}

SimpleScene::~SimpleScene()
{
    clearOwnedElements();
}

void SimpleScene::clearOwnedElements()
{
    for (long int i = 0; i < simpleBodies.size(); ++i) {
        delete simpleBodies[i];
    }
    for (long int i = 0; i < lights.size(); ++i) {
        delete lights[i];
    }
    for (long int i = 0; i < backgrounds.size(); ++i) {
        delete backgrounds[i];
    }
    for (long int i = 0; i < cameras.size(); ++i) {
        delete cameras[i];
    }

    simpleBodies.clear();
    lights.clear();
    backgrounds.clear();
    cameras.clear();

    activeCameraIndex = 0;
    activeBackgroundIndex = 0;
}

int SimpleScene::getActiveCameraIndex() const { return activeCameraIndex; }
int SimpleScene::getActiveBackgroundIndex() const { return activeBackgroundIndex; }
void SimpleScene::setActiveCameraIndex(int i) { activeCameraIndex = i; }
void SimpleScene::setActiveBackgroundIndex(int i) { activeBackgroundIndex = i; }

void SimpleScene::addBody(SimpleBody* b) { simpleBodies.add(b); }
void SimpleScene::addCamera(Camera* c) { cameras.add(c); }
void SimpleScene::addBackground(Background* b) { backgrounds.add(b); }
void SimpleScene::addLight(Light* l) { l->setId(static_cast<int>(lights.size())); lights.add(l); }

java::ArrayList<SimpleBody*>& SimpleScene::getSimpleBodies() { return simpleBodies; }
java::ArrayList<Light*>& SimpleScene::getLights() { return lights; }
java::ArrayList<Background*>& SimpleScene::getBackgrounds() { return backgrounds; }
java::ArrayList<Camera*>& SimpleScene::getCameras() { return cameras; }

void SimpleScene::setSimpleBodies(java::ArrayList<SimpleBody*>& simpleBodies)
{
    for (long int i = 0; i < this->simpleBodies.size(); ++i) {
        delete this->simpleBodies[i];
    }
    this->simpleBodies.clear();
    for (long int i = 0; i < simpleBodies.size(); i++) this->simpleBodies.add(simpleBodies.get(i));
}
void SimpleScene::setLights(java::ArrayList<Light*>& lights)
{
    for (long int i = 0; i < this->lights.size(); ++i) {
        delete this->lights[i];
    }
    this->lights.clear();
    for (long int i = 0; i < lights.size(); i++) this->lights.add(lights.get(i));
    for (long int i = 0; i < this->lights.size(); ++i) {
        this->lights[i]->setId(static_cast<int>(i));
    }
}
void SimpleScene::setBackgrounds(java::ArrayList<Background*>& backgrounds)
{
    for (long int i = 0; i < this->backgrounds.size(); ++i) {
        delete this->backgrounds[i];
    }
    this->backgrounds.clear();
    for (long int i = 0; i < backgrounds.size(); i++) this->backgrounds.add(backgrounds.get(i));
}
void SimpleScene::setCameras(java::ArrayList<Camera*>& cameras)
{
    for (long int i = 0; i < this->cameras.size(); ++i) {
        delete this->cameras[i];
    }
    this->cameras.clear();
    for (long int i = 0; i < cameras.size(); i++) this->cameras.add(cameras.get(i));
}

Background* SimpleScene::getActiveBackground() const { return backgrounds.get(activeBackgroundIndex); }
Camera* SimpleScene::getActiveCamera() const { return cameras.get(activeCameraIndex); }

SimpleSceneSnapshot* SimpleScene::exportToSimpleSceneSnapshot()
{
    return exportToSimpleSceneSnapshot(getActiveCamera()->exportToCameraSnapshot(), getActiveBackground());
}

SimpleSceneSnapshot* SimpleScene::exportToSimpleSceneSnapshot(int viewportXSize, int viewportYSize)
{
    return exportToSimpleSceneSnapshot(getActiveCamera()->exportToCameraSnapshot(viewportXSize, viewportYSize), getActiveBackground());
}

SimpleSceneSnapshot* SimpleScene::exportToSimpleSceneSnapshot(CameraSnapshot* cameraSnapshot, Background* background)
{
    return new SimpleSceneSnapshot(simpleBodies, lights, background, cameraSnapshot);
}
