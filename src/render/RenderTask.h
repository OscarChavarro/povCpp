#ifndef __RENDER_TASK__
#define __RENDER_TASK__

#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"
#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "render/RenderWorker.h"

class RenderTask {
  public:
    RenderWorker worker;
    IntersectionPriorityQueuePool pool;
    RasterTileArea area;
    Statistics statistics;
    TextureUtils textureUtils;

    RenderTask(RenderEngine *engine, const RasterTileArea &taskArea)
        : worker(engine), area(taskArea)
    {
        pool.init();
    }
};

#endif
