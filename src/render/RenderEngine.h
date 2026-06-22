#ifndef __RENDER_ENGINE__
#define __RENDER_ENGINE__

#include <atomic>

#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "environment/scene/Scene.h"
#include "render/AdaptiveAntiAliasing.h"
#include "render/RenderContext.h"
#include "render/RenderImageWriter.h"
#include "render/RenderTargetImage.h"
#include "render/RenderWorker.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"

class RenderEngine {
  private:
    RenderContext *context;
    Scene *scene;
    double *maxTraceLevelValue;
    bool *stopFlagValue;
    RenderWorker worker;
    IntersectionPriorityQueuePool intersectionQueuePool;
    AdaptiveAntiAliasing adaptiveAntiAliasing;
    RenderImageWriter imageWriter;
    RenderTargetImage destinationImage;
    RasterTileArea renderArea;
    std::atomic<bool> fatalErrorFound;

    void copyLineToImage(
        const ColorRgba *line, int row, const RasterTileArea &area);
    void persistDestinationImage();

  public:
    RenderEngine();
    ~RenderEngine();

    void setContext(RenderContext *ctx);
    void setMaxTraceLevel(double &value);
    void setStopFlag(bool &value);
    const RenderingConfiguration &getConfig() const;
    RenderingConfiguration &getMutableConfig();
    const RenderContext &getRenderContext() const;
    Statistics &getStatistics();
    IntersectionPriorityQueuePool &getIntersectionQueuePool();
    void setScene(Scene *s);
    Scene &getScene();
    double &getMaxTraceLevel();
    bool &getStopFlag();
    void readRenderedPart(void);
    void startTracing(void);
    void startTracingParallel(void);
    void trace(RenderWorker &localWorker, RayWithSegments *localRay, ColorRgba *color);
    void initializeRenderer(void);
    void createRay(RayWithSegments *localRay, int width, int height, double x,
        double y, IntersectionPriorityQueuePool *pool, Statistics *stats);
    void checkStats(int y);
    void renderTile(
        RenderWorker &localWorker, IntersectionPriorityQueuePool &pool,
        Statistics &stats, const RasterTileArea &area);
};

inline
RenderEngine::RenderEngine()
    : context(nullptr), scene(nullptr), maxTraceLevelValue(nullptr),
      stopFlagValue(nullptr), worker(this),
      adaptiveAntiAliasing(this),
      imageWriter(this),
      fatalErrorFound(false)
{
}

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

inline const RenderingConfiguration &
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

inline IntersectionPriorityQueuePool &
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
