#ifndef __BAKED_SCENE_BUILDER__
#define __BAKED_SCENE_BUILDER__

#include "environment/scene/SimpleBody.h"
#include "render/bakedScene/BakedScene.h"

// Plan 6 Phase 4: builds the baked model directly from the parsed
// SimpleBody/CsgOperand/Composite tree - environment/scene owns only the
// parsed data (Scene::getObjects()) and no longer builds or owns any baked
// representation itself. `out` is caller-owned, stable storage (e.g.
// RenderEngine's own BakedScene member) so the self-referential
// bakedQuadric/bakedPlane pointers set up during the build's final fix-up
// pass never have to survive a return-by-value copy/move.
class BakedSceneBuilder {
  public:
    static void build(const java::ArrayList<SimpleBody*> &objects, BakedScene &out);
};

#endif
