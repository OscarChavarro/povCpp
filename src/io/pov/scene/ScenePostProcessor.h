#ifndef __SCENE_POST_PROCESSOR__
#define __SCENE_POST_PROCESSOR__

#include "environment/scene/SimpleBody.h"
#include "environment/light/Light.h"
#include "java/util/ArrayList.h"

class ScenePostProcessor {
  public:
    static void linkLights(SimpleBody *object, java::ArrayList<Light*> &lights);

  private:
    static void linkLightsInShape(Geometry *shape, java::ArrayList<Light*> &lights);
};

#endif
