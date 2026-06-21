#ifndef __RENDER_ENGINE__
#define __RENDER_ENGINE__

#include <atomic>

#include "java/util/concurrent/Callable.h"
#include "java/util/concurrent/Void.h"

#include "common/RenderRuntimeState.h"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "environment/scene/Scene.h"
#include "render/AdaptiveAntiAliasing.h"
#include "render/RenderContext.h"
#include "render/RenderImageWriter.h"
#include "render/RenderTargetImage.h"
#include "render/RenderWorker.h"
#include "vsdk/toolkit/render/raytracing/RasterTileArea.h"

class RenderTask;

class RenderEngine {
  private:
    RenderContext *context;
    Scene *scene;
    RenderWorker worker;
    IntersectionPriorityQueuePool intersectionQueuePool;
    AdaptiveAntiAliasing adaptiveAntiAliasing;
    // Owns the file-I/O half of the render (persist / resume scanlines), kept
    // apart from the sampling driver below so the driver can be swapped without
    // dragging output formatting along.
    RenderImageWriter imageWriter;
    // Full-frame render target and the region of it this engine currently
    // owns. Serial mode always covers the whole image; tile-based parallel
    // mode (future) sets this to each thread's band.
    RenderTargetImage destinationImage;
    RasterTileArea renderArea;
    // Set the first time a fatal abort is detected, so the error is reported
    // only once instead of per pixel, even when several RenderTask threads
    // observe the abort concurrently (B8: atomic report-once via CAS).
    // Rendering keeps going with a default (black) colour instead of
    // terminating the whole process.
    std::atomic<bool> fatalErrorFound;

  public:
    RenderEngine();
    ~RenderEngine();

    void setContext(RenderContext *ctx);
    const RenderingConfiguration &getConfig() const;
    RenderingConfiguration &getMutableConfig();
    const RenderContext &getRenderContext() const;
    Statistics &getStatistics();
    TextureUtils &getTextureUtils();
    IntersectionPriorityQueuePool &getIntersectionQueuePool();
    RenderWorker &getWorker();
    void setScene(Scene *s);
    Scene &getScene();
    double &getMaxTraceLevel();
    bool &getStopFlag();
    void readRenderedPart(void);
    void startTracing(void);
    // Tile-based parallel driver (B1-B4/B7/B9): splits renderArea into
    // getConfig().getNumberOfThreads() horizontal bands via RasterTileGenerator,
    // renders each band on its own RenderTask (own worker/pool/statistics) in
    // a fixed thread pool, joins, reduces statistics, then persists in order.
    void startTracingParallel(void);
    RenderTargetImage &getDestinationImage() { return destinationImage; }
    const RasterTileArea &getRenderArea() const { return renderArea; }
    void setRenderArea(const RasterTileArea &area) { renderArea = area; }
    void trace(RenderWorker &localWorker, RayWithSegments *localRay, ColorRgba *color);
    void initializeRenderer(void);
    void createRay(RayWithSegments *localRay, int width, int height, double x,
        double y, IntersectionPriorityQueuePool *pool, Statistics *stats);
    void checkStats(int y);

    // Render exactly the rows/columns described by `area` into
    // destinationImage, using `localWorker` for ray/line-buffer scratch,
    // `pool` for intersection queues and `stats` for pixel/ray/intersection
    // counters. Independent RenderTask instances (each owning their own
    // worker + pool + statistics) can call this concurrently on disjoint
    // areas without sharing any mutable state.
    void renderTile(
        RenderWorker &localWorker, IntersectionPriorityQueuePool &pool,
        Statistics &stats, const RasterTileArea &area);

  private:
    // Copy one finished scanline (already final, post-AA) into the
    // destination image, restricted to the given area's columns.
    void copyLineToImage(
        const ColorRgba *line, int row, const RasterTileArea &area);
    // Persist every row of the destination image to disk, in increasing row
    // order, once the whole render area is finished.
    void persistDestinationImage();

    // One renderTile() call on one thread, wrapped so it can be submitted to
    // a java::ExecutorService. Owns nothing; both engine and task outlive
    // every task's run (they are joined before startTracingParallel
    // returns).
    class RenderTileCallable : public java::Callable<java::Void> {
      private:
        RenderEngine *engine;
        RenderTask *task;

      public:
        RenderTileCallable(RenderEngine *engineIn, RenderTask *taskIn)
            : engine(engineIn), task(taskIn) {}

        java::Void call() override;
    };
};

inline
RenderEngine::RenderEngine()
    : context(nullptr), scene(nullptr), worker(this),
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

inline TextureUtils &
RenderEngine::getTextureUtils()
{
    return context->getTextureUtils();
}

inline IntersectionPriorityQueuePool &
RenderEngine::getIntersectionQueuePool()
{
    return intersectionQueuePool;
}

inline RenderWorker &
RenderEngine::getWorker()
{
    return worker;
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
    return context->getRuntime().getMaxTraceLevel();
}

inline bool &
RenderEngine::getStopFlag()
{
    return context->getRuntime().getStopFlag();
}

#endif
