#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

#include "environment/geometry/TransformedGeometry.h"
#include "environment/scene/SimpleBody.h"

class SceneBuilder {
  public:
    static SimpleBody *wrap(TransformedGeometry *geometry);
};

#endif
