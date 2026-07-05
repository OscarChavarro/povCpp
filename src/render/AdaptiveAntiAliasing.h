#ifndef __ADAPTIVE_ANTI_ALIASING__
#define __ADAPTIVE_ANTI_ALIASING__

#include "render/shaders/PovRayRenderStatistics.h"
#include "environment/geometry/element/PriorityQueuePool.h"
#include "render/RenderWorker.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"

class RenderEngine;
class IntersectionCandidate;

class AdaptiveAntiAliasing {
  private:
    static constexpr int SUPER_SAMPLE_GRID_SIZE = 3;
    static constexpr int SUPER_SAMPLE_COUNT = SUPER_SAMPLE_GRID_SIZE * SUPER_SAMPLE_GRID_SIZE;
    static constexpr int JITTER_SEED_INITIAL_OFFSET = 10;
    static constexpr int JITTER_SEED_INCREMENT = 10;
    static constexpr int JITTER_RANDOM_MASK = 0x7FFF;
    static constexpr double JITTER_RANDOM_NORMALIZER = 32768.0;
    static constexpr double SUPER_SAMPLE_CELL_SIZE = 1.0 / 3.0;
    static constexpr double SUPER_SAMPLE_JITTER_RANGE = SUPER_SAMPLE_CELL_SIZE;
    static constexpr double SUPER_SAMPLE_JITTER_BIAS = SUPER_SAMPLE_JITTER_RANGE / 2.0;
    static constexpr double SUPER_SAMPLE_WEIGHT = 1.0 / SUPER_SAMPLE_COUNT;

    RenderEngine *renderEngine;

    inline unsigned short rand3dInline(RenderWorker &worker, int a, int b);
    void superSample(
        RenderWorker &worker, ColorRgba *result, int x, int y, int width,
        int height, PriorityQueuePool<IntersectionCandidate> *pool, PovRayRenderStatistics *stats);

  public:
    explicit AdaptiveAntiAliasing(RenderEngine *engine)
        : renderEngine(engine) {}

    void doAntiAliasing(RenderWorker &worker, int x, int y, ColorRgba *color,
        const RasterTileArea &area, PriorityQueuePool<IntersectionCandidate> *pool,
        PovRayRenderStatistics *stats);
};

#endif
