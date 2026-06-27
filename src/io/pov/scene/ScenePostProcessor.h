#ifndef __SCENE_POST_PROCESSOR__
#define __SCENE_POST_PROCESSOR__

#include "environment/geometry/BoundedGeometry.h"
#include "environment/light/Light.h"
#include "java/util/ArrayList.h"

class ScenePostProcessor {
  public:
    static void linkLights(BoundedGeometry *object, java::ArrayList<Light*> &lights);

  private:
    static void linkLightsInShape(TransformedGeometry *shape, java::ArrayList<Light*> &lights);
};

#endif
