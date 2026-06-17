#ifndef __RENDER_ENGINE_H__
#define __RENDER_ENGINE_H__

#include "environment/scene/SceneFrame.h"
#include "render/shaders/TraceService.h"

class RenderEngine {
  private:
    static RenderFrame sRenderFrame;
    static RayWithSegments *sPrimaryRay;
    static int sTraceLevel;
    static void traceServiceTrace(void *context, RayWithSegments *ray, ColorRgba *color);
    static void traceServiceShadeShadow(void *context, Intersection *intersection, ColorRgba *color);

  public:
    static RenderFrame &renderFrame();
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
};

#endif
