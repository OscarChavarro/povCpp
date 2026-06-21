#ifndef __RENDER_TILE_CALLABLE__
#define __RENDER_TILE_CALLABLE__

#include "java/util/concurrent/Callable.h"
#include "java/util/concurrent/Void.h"

class RenderTileCallable : public java::Callable<java::Void> {
  private:
    RenderEngine *engine;
    RenderTask *task;

  public:
    RenderTileCallable(RenderEngine *engineIn, RenderTask *taskIn)
        : engine(engineIn), task(taskIn) {}

    java::Void call() override;
};

#endif
