#ifndef __IO_SCENE_BUILDER__
#define __IO_SCENE_BUILDER__

#include "environment/geometry/TransformedGeometry.h"
#include "io/pov/geometry/SimpleBodyBuilder.h"

class SceneBuilder {
  public:
    static SimpleBodyBuilder *wrap(TransformedGeometry *geometry);
};

#endif
