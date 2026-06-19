#ifndef __SCENE_POST_PROCESSOR__
#define __SCENE_POST_PROCESSOR__

#include "environment/light/Light.h"
#include "environment/geometry/BoundedGeometry.h"

class SimpleBody;

class ScenePostProcessor {
  public:
    static void linkLights(BoundedGeometry *object, Light *&lightHead);

  private:
    static void linkLightsInShape(SimpleBody *shape, Light *&lightHead);
};

#endif
