#ifndef __RENDER_ENGINE__
#define __RENDER_ENGINE__

#include "common/dataStructures/IntersectionPriorityQueuePool.h"
#include "environment/scene/Scene.h"
#include "render/shaders/TraceService.h"
#include "render/RenderContext.h"

class RenderEngine {
  private:
    RenderContext *mContext;

    Scene *mScene;
    RayWithSegments *mPrimaryRay;
    int mTraceLevel;
    int mSuperSampleCount;
    ColorRgba *mPreviousLine;
    ColorRgba *mCurrentLine;
    char *mPreviousLineAntialiasedFlags;
    char *mCurrentLineAntialiasedFlags;
    RayWithSegments mRay;
    TraceService mTraceService;
    IntersectionPriorityQueuePool mIntersectionQueuePool;

    static void traceServiceTrace(void *context, RayWithSegments *ray, ColorRgba *color);
    static void traceServiceShadeShadow(void *context, Intersection *intersection, ColorRgba *color);

  public:
    RenderEngine()
        : mContext(nullptr), mScene(nullptr), mPrimaryRay(nullptr), mTraceLevel(0), mSuperSampleCount(0),
          mPreviousLine(nullptr), mCurrentLine(nullptr),
          mPreviousLineAntialiasedFlags(nullptr), mCurrentLineAntialiasedFlags(nullptr),
          mTraceService(RenderEngine::traceServiceTrace, RenderEngine::traceServiceShadeShadow, this) {}
    ~RenderEngine();

    void setContext(RenderContext *ctx) { mContext = ctx; }
    RenderContext *getContext() { return mContext; }
    const RenderingConfiguration &getConfig() const { return mContext->getConfig(); }
    RenderingConfiguration &getMutableConfig();
    Statistics &getStatistics() { return mContext->getStatistics(); }
    TextureUtils &getTextureUtils() { return mContext->getTextureUtils(); }
    IntersectionPriorityQueuePool &getIntersectionQueuePool() { return mIntersectionQueuePool; }

    void setScene(Scene *scene) { mScene = scene; }
    Scene &scene() { return *mScene; }
    RayWithSegments *&primaryRay() { return mPrimaryRay; }
    int &traceLevel() { return mTraceLevel; }
    double &maxTraceLevel() { return mContext->getRuntime().getMaxTraceLevel(); }
    volatile int &stopFlag() { return mContext->getRuntime().getStopFlag(); }

    void readRenderedPart(void);
    void supersample(
        ColorRgba *result, int x, int y, int width, int height);
    void startTracing(void);
    void trace(RayWithSegments *ray, ColorRgba *color);
    void initializeRenderer(void);
    inline unsigned short rand3dInline(int a, int b);
    const TraceService *getTraceService() { return &mTraceService; }

    void createRay(
        RayWithSegments *ray, int width, int height, double x, double y);
    void checkStats(int y);
    void doAntiAliasing(int x, int y, ColorRgba *color);
    void outputLine(int y);
};

#endif
