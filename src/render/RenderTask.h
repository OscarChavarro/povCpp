#ifndef __RENDER_TASK__
#define __RENDER_TASK__

#include "environment/geometry/element/IntersectionCandidate.h"

class RenderTask {
  public:
    RenderWorker worker;
    PriorityQueuePool<IntersectionCandidate> pool;
    RasterTileArea area;
    PovRayRenderStatistics statistics;
    TextureUtils textureUtils;

    RenderTask(RenderEngine *engine, const RasterTileArea &taskArea)
        : worker(engine), area(taskArea)
    {
        pool.init();
    }
};

#endif
