#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

class Geometry;
class SimpleBody;

class SceneBuilder {
  public:
    static SimpleBody *wrap(Geometry *geometry);
};

#endif
