#ifndef __RENDER_ENGINE__
#define __RENDER_ENGINE__

#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "environment/scene/Scene.h"
#include "render/AdaptiveAntiAliasing.h"
#include "render/RenderContext.h"
#include "render/shaders/TraceService.h"

class RenderEngine {
  private:
    RenderContext *context;
    Scene *scene;
    RayWithSegments *primaryRay;
    int traceLevel;
    int superSampleCount;
    ColorRgba *previousLine;
    ColorRgba *currentLine;
    char *previousLineAntiAliasedFlags;
    char *currentLineAntiAliasedFlags;
    RayWithSegments ray;
    TraceService traceService;
    IntersectionPriorityQueuePool intersectionQueuePool;
    AdaptiveAntiAliasing adaptiveAntiAliasing;
    // Set the first time a fatal abort is detected on the thread driving this
    // engine, so the error is reported only once instead of per pixel. Not
    // reentrant across threads, but non-blocking: rendering keeps going with a
    // default (black) colour instead of terminating the whole process.
    bool fatalErrorFound;

    static ColorRgba *allocateColorBuffer(int count);
    static void traceServiceTrace(void *context, RayWithSegments *ray, ColorRgba *color);
    static void traceServiceShadeShadow(void *context, Intersection *intersection, ColorRgba *color);

  public:
    RenderEngine()
        : context(nullptr), scene(nullptr), primaryRay(nullptr), traceLevel(0), superSampleCount(0),
          previousLine(nullptr), currentLine(nullptr),
          previousLineAntiAliasedFlags(nullptr), currentLineAntiAliasedFlags(nullptr),
          traceService(RenderEngine::traceServiceTrace, RenderEngine::traceServiceShadeShadow, this),
          adaptiveAntiAliasing(this),
          fatalErrorFound(false) {}
    ~RenderEngine();

    void setContext(RenderContext *ctx) { context = ctx; }
    const RenderingConfiguration &getConfig() const { return context->getConfig(); }
    RenderingConfiguration &getMutableConfig();
    Statistics &getStatistics() { return context->getStatistics(); }
    TextureUtils &getTextureUtils() { return context->getTextureUtils(); }
    IntersectionPriorityQueuePool &getIntersectionQueuePool() { return intersectionQueuePool; }
    RayWithSegments *getPrimaryRay() { return primaryRay; }
    void setTraceLevel(int level) { traceLevel = level; }
    ColorRgba *getPreviousLinePixel(int x) { return &previousLine[x]; }
    ColorRgba *getCurrentLinePixel(int x) { return &currentLine[x]; }
    bool getPreviousLineAntiAliasedFlag(int x) const {
        return previousLineAntiAliasedFlags[x] != 0;
    }
    void setPreviousLineAntiAliasedFlag(int x, bool value) {
        previousLineAntiAliasedFlags[x] = value ? 1 : 0;
    }
    bool getCurrentLineAntiAliasedFlag(int x) const {
        return currentLineAntiAliasedFlags[x] != 0;
    }
    void setCurrentLineAntiAliasedFlag(int x, bool value) {
        currentLineAntiAliasedFlags[x] = value ? 1 : 0;
    }
    void incrementSuperSampleCount() { superSampleCount++; }
    void setScene(Scene *s) { scene = s; }
    Scene &getScene() { return *scene; }
    double &getMaxTraceLevel() { return context->getRuntime().getMaxTraceLevel(); }
    bool &getStopFlag() { return context->getRuntime().getStopFlag(); }
    void readRenderedPart(void);
    void startTracing(void);
    void trace(RayWithSegments *localRay, ColorRgba *color);
    void initializeRenderer(void);
    const TraceService *getTraceService() { return &traceService; }
    void createRay(
        RayWithSegments *localRay, int width, int height, double x, double y);
    void checkStats(int y);
    void outputLine(int y);
};

#endif
