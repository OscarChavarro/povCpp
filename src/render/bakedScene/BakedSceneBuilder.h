#ifndef __BAKED_SCENE_BUILDER__
#define __BAKED_SCENE_BUILDER__

#include "environment/scene/Scene.h"
#include "render/bakedScene/BakedScene.h"

// Plan 6 Phase 1: translates the (still authoritative) Scene::CompiledTracingScene
// - the old, per-ray-re-classifying model - into the new flat BakedScene model.
// Phases 2-3 will make render/bakedScene's trace entry points read the new
// model instead of the old one; Phase 4 retires Scene::CompiledTracingScene
// and its Baked* structs entirely and moves this translation's logic to
// build directly from the parsed SimpleBody/CsgOperand/Composite tree
// (mirroring what Scene.cpp's bakeSimpleBody/bakeCsgOperand/bakeComposite do
// today), so this class stops needing the old model as an intermediate step.
class BakedSceneBuilder {
  public:
    static BakedScene build(const Scene::CompiledTracingScene &compiledScene);
};

#endif
