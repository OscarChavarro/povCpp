#ifndef __RENDER_ENGINE__
#define __RENDER_ENGINE__

#include "common/RenderRuntimeState.h"
#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "environment/scene/Scene.h"
#include "render/AdaptiveAntiAliasing.h"
#include "render/RenderContext.h"
#include "render/RenderWorker.h"

class RenderEngine {
  private:
    RenderContext *context;
    Scene *scene;
    int superSampleCount;
    RenderWorker worker;
    IntersectionPriorityQueuePool intersectionQueuePool;
    AdaptiveAntiAliasing adaptiveAntiAliasing;
    // Set the first time a fatal abort is detected on the thread driving this
    // engine, so the error is reported only once instead of per pixel. Not
    // reentrant across threads, but non-blocking: rendering keeps going with a
    // default (black) colour instead of terminating the whole process.
    bool fatalErrorFound;

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
    void incrementSuperSampleCount();
    void setScene(Scene *s);
    Scene &getScene();
    double &getMaxTraceLevel();
    bool &getStopFlag();
    void readRenderedPart(void);
    void startTracing(void);
    void trace(RenderWorker &localWorker, RayWithSegments *localRay, ColorRgba *color);
    void initializeRenderer(void);
    void createRay(
        RayWithSegments *localRay, int width, int height, double x, double y);
    void checkStats(int y);
    void outputLine(RenderWorker &localWorker, int y);
};

inline
RenderEngine::RenderEngine()
    : context(nullptr), scene(nullptr), superSampleCount(0), worker(this),
      adaptiveAntiAliasing(this),
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
RenderEngine::incrementSuperSampleCount()
{
    superSampleCount++;
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
