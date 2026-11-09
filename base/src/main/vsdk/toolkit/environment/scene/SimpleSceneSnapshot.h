#ifndef __SIMPLE_SCENE_SNAPSHOT__
#define __SIMPLE_SCENE_SNAPSHOT__

#include "vsdk/toolkit/common/Entity.h"
#include "java/util/ArrayList.h"
class SimpleBody;
class Light;
class Background;
class CameraSnapshot;

class SimpleSceneSnapshot : public Entity {
private:
    java::ArrayList<SimpleBody*> simpleBodies;
    java::ArrayList<Light*> lights;
    Background* background;
    CameraSnapshot* cameraSnapshot;

public:
    SimpleSceneSnapshot(
        java::ArrayList<SimpleBody*>& simpleBodies,
        java::ArrayList<Light*>& lights,
        Background* background,
        CameraSnapshot* cameraSnapshot);

    ~SimpleSceneSnapshot();

    java::ArrayList<SimpleBody*>& getSimpleBodies();
    java::ArrayList<Light*>& getLights();
    Background* getBackground() const;
    CameraSnapshot* getCameraSnapshot() const;
};

#endif
