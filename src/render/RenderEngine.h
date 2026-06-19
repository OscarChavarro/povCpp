#ifndef __RENDER_ENGINE__
#define __RENDER_ENGINE__

#include "environment/scene/Scene.h"
#include "render/shaders/TraceService.h"

class RenderEngine {
  private:
    static RenderEngine *sActive;

    Scene *mScene;
    RayWithSegments *mPrimaryRay;
    int mTraceLevel;
    int mSuperSampleCount;
    ColorRgba *mPreviousLine;
    ColorRgba *mCurrentLine;
    char *mPreviousLineAntialiasedFlags;
    char *mCurrentLineAntialiasedFlags;
    RayWithSegments mRay;

    static void traceServiceTrace(void *context, RayWithSegments *ray, ColorRgba *color);
    static void traceServiceShadeShadow(void *context, Intersection *intersection, ColorRgba *color);

  public:
    RenderEngine()
        : mScene(nullptr), mPrimaryRay(nullptr), mTraceLevel(0), mSuperSampleCount(0),
          mPreviousLine(nullptr), mCurrentLine(nullptr),
          mPreviousLineAntialiasedFlags(nullptr), mCurrentLineAntialiasedFlags(nullptr) {}
    ~RenderEngine();

    static void installActive(RenderEngine *engine) { sActive = engine; }
    static RenderEngine *active() { return sActive; }
    friend class Scene;

    static void setScene(Scene *scene) { if (sActive) sActive->mScene = scene; }
    static Scene &scene();
    static RayWithSegments *&primaryRay();
    static int &traceLevel();
    static double &maxTraceLevel();
    static volatile int &stopFlag();

    static void readRenderedPart(void);
    static void supersample(
        ColorRgba *result, int x, int y, int width, int height);
    static void startTracing(void);
    static void trace(RayWithSegments *ray, ColorRgba *color);
    static void initializeRenderer(void);
    static inline unsigned short rand3dInline(int a, int b);
    static const TraceService *getTraceService();

    static void createRay(
        RayWithSegments *ray, int width, int height, double x, double y);
    static void checkStats(int x, int y);
    static void doAntiAliasing(int x, int y, ColorRgba *color);
    static void outputLine(int y);
};

#endif
