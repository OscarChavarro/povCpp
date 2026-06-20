#ifndef __RENDER_ENGINE__
#define __RENDER_ENGINE__

#include "environment/geometry/element/IntersectionPriorityQueuePool.h"
#include "environment/scene/Scene.h"
#include "render/shaders/TraceService.h"
#include "render/RenderContext.h"

class RenderEngine {
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
          fatalErrorFound(false) {}
    ~RenderEngine();

    void setContext(RenderContext *ctx) { context = ctx; }
    const RenderingConfiguration &getConfig() const { return context->getConfig(); }
    RenderingConfiguration &getMutableConfig();
    Statistics &getStatistics() { return context->getStatistics(); }
    TextureUtils &getTextureUtils() { return context->getTextureUtils(); }
    IntersectionPriorityQueuePool &getIntersectionQueuePool() { return intersectionQueuePool; }
    void setScene(Scene *s) { scene = s; }
    Scene &getScene() { return *scene; }
    double &getMaxTraceLevel() { return context->getRuntime().getMaxTraceLevel(); }
    bool &getStopFlag() { return context->getRuntime().getStopFlag(); }
    void readRenderedPart(void);
    void superSample(
        ColorRgba *result, int x, int y, int width, int height);
    void startTracing(void);
    void trace(RayWithSegments *ray, ColorRgba *color);
    void initializeRenderer(void);
    inline unsigned short rand3dInline(int a, int b);
    const TraceService *getTraceService() { return &traceService; }
    void createRay(
        RayWithSegments *ray, int width, int height, double x, double y);
    void checkStats(int y);
    void doAntiAliasing(int x, int y, ColorRgba *color);
    void outputLine(int y);
};

#endif
