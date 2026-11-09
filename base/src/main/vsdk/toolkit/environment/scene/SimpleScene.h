#ifndef __SIMPLE_SCENE__
#define __SIMPLE_SCENE__

#include "vsdk/toolkit/common/Entity.h"
#include "java/util/ArrayList.h"
class SimpleBody;
class Light;
class Background;
class Camera;
class CameraSnapshot;
class SimpleSceneSnapshot;

class SimpleScene : public Entity {
private:
    java::ArrayList<SimpleBody*> simpleBodies;
    java::ArrayList<Light*> lights;
    java::ArrayList<Background*> backgrounds;
    java::ArrayList<Camera*> cameras;
    int activeCameraIndex;
    int activeBackgroundIndex;

public:
    SimpleScene();
    virtual ~SimpleScene();

    void clearOwnedElements();

    int getActiveCameraIndex() const;
    int getActiveBackgroundIndex() const;
    void setActiveCameraIndex(int i);
    void setActiveBackgroundIndex(int i);

    void addBody(SimpleBody* b);
    void addCamera(Camera* c);
    void addBackground(Background* b);
    void addLight(Light* l);

    java::ArrayList<SimpleBody*>& getSimpleBodies();
    const java::ArrayList<SimpleBody*>& getSimpleBodies() const { return simpleBodies; }
    java::ArrayList<Light*>& getLights();
    const java::ArrayList<Light*>& getLights() const { return lights; }
    java::ArrayList<Background*>& getBackgrounds();
    java::ArrayList<Camera*>& getCameras();

    void setSimpleBodies(java::ArrayList<SimpleBody*>& simpleBodies);
    void setLights(java::ArrayList<Light*>& lights);
    void setBackgrounds(java::ArrayList<Background*>& backgrounds);
    void setCameras(java::ArrayList<Camera*>& cameras);

    Background* getActiveBackground() const;
    Camera* getActiveCamera() const;

    virtual SimpleSceneSnapshot* exportToSimpleSceneSnapshot();
    virtual SimpleSceneSnapshot* exportToSimpleSceneSnapshot(int viewportXSize, int viewportYSize);
    SimpleSceneSnapshot* exportToSimpleSceneSnapshot(CameraSnapshot* cameraSnapshot, Background* background);
};

#endif
