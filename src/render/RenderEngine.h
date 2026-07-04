#ifndef __RENDER_ENGINE__
#define __RENDER_ENGINE__

#include <atomic>

#include "environment/geometry/element/PriorityQueuePool.h"
#include "environment/scene/Scene.h"
#include "render/AdaptiveAntiAliasing.h"
#include "render/RenderContext.h"
#include "render/RenderImageWriter.h"
#include "render/RenderTargetImage.h"
#include "render/RenderWorker.h"
#include "render/bakedScene/BakedScene.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"

class IntersectionCandidate;

class RenderEngine {
  private:
    RenderContext *context;
    Scene *scene;
    double *maxTraceLevelValue;
    bool *stopFlagValue;
    RenderWorker worker;
    PriorityQueuePool<IntersectionCandidate> intersectionQueuePool;
    AdaptiveAntiAliasing adaptiveAntiAliasing;
    RenderImageWriter imageWriter;
    RenderTargetImage destinationImage;
    RasterTileArea renderArea;
    std::atomic<bool> fatalErrorFound;
    // Plan 6 Phase 4: RenderEngine owns the baked model - environment/scene
    // only owns parsed data now. Built once, after parsing finishes (see
    // buildBakedScene()), read-only for the rest of the render (including
    // every -parallel worker, via RenderContext::getBakedScene()).
    BakedScene bakedScene;

    void copyLineToImage(
        const ColorRgba *line, int row, const RasterTileArea &area);
    void persistDestinationImage();

  public:
    RenderEngine();
    ~RenderEngine();

    void setContext(RenderContext *ctx);
    void setMaxTraceLevel(double &value);
    void setStopFlag(bool &value);
    const PovRayRendererConfiguration &getConfig() const;
    PovRayRendererConfiguration &getMutableConfig();
    const RenderContext &getRenderContext() const;
    Statistics &getStatistics();
    PriorityQueuePool<IntersectionCandidate> &getIntersectionQueuePool();
    void setScene(Scene *s);
    Scene &getScene();
    void buildBakedScene();
    const BakedScene &getBakedScene() const { return bakedScene; }
    double &getMaxTraceLevel();
    bool &getStopFlag();
    void readRenderedPart(void);
    void startTracing(void);
    void startTracingParallel(void);
    void trace(RenderWorker &localWorker, RayWithSegments *localRay, ColorRgba *color);
    void initializeRenderer(void);
    void createRay(RayWithSegments *localRay, int width, int height, double x,
        double y, PriorityQueuePool<IntersectionCandidate> *pool, Statistics *stats);
    void checkStats(int y);
    void renderTile(
        RenderWorker &localWorker, PriorityQueuePool<IntersectionCandidate> &pool,
        Statistics &stats, const RasterTileArea &area);
};

inline void
RenderEngine::setContext(RenderContext *ctx)
{
    context = ctx;
}

inline void
RenderEngine::setMaxTraceLevel(double &value)
{
    maxTraceLevelValue = &value;
}

inline void
RenderEngine::setStopFlag(bool &value)
{
    stopFlagValue = &value;
}

inline const PovRayRendererConfiguration &
RenderEngine::getConfig() const
{
    return context->getConfig();
}

inline const RenderContext &
RenderEngine::getRenderContext() const
{
    return *context;
}

inline Statistics &
RenderEngine::getStatistics()
{
    return context->getStatistics();
}

inline PriorityQueuePool<IntersectionCandidate> &
RenderEngine::getIntersectionQueuePool()
{
    return intersectionQueuePool;
}

inline void
RenderEngine::setScene(Scene *s)
{
    scene = s;
}

inline Scene &
RenderEngine::getScene()
{
    return *scene;
}

inline double &
RenderEngine::getMaxTraceLevel()
{
    return *maxTraceLevelValue;
}

inline bool &
RenderEngine::getStopFlag()
{
    return *stopFlagValue;
}

#endif
