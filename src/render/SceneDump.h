#ifndef __SCENE_DUMPER__
#define __SCENE_DUMPER__

#include <cstdio>

class Scene;

class SceneDumper {
  public:
    static void dumpSceneStructure(FILE *f, const Scene &scene);
};

#endif
