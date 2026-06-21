#ifndef __RENDER_TASK__
#define __RENDER_TASK__

#include "common/statistics/Statistics.h"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "render/RenderWorker.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"

class RenderEngine;

// Everything one rendering thread mutates while tracing its own tile: ray
// scratch + line buffers (RenderWorker), its own intersection queue pool (so
// concurrent CSG/composite/poly intersection tests never share queues across
// threads), its own Statistics (summed into the shared total at join time via
// Statistics(ArrayList<Statistics*>*), see RenderEngine::startTracingParallel),
// its own TextureUtils (so noise()/differentialNoise() call counters in
// SolidTextureStatistics never race across threads either — reduced the same
// way, via SolidTextureStatistics(ArrayList<SolidTextureStatistics*>*)), and
// the RasterTileArea it is responsible for. One RenderTask per thread; the
// serial driver just uses a single full-frame instance.
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
