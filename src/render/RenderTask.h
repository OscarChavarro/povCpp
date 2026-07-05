#ifndef __RENDER_TASK__
#define __RENDER_TASK__


class IntersectionCandidate;

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
