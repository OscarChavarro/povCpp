#ifndef __RENDER_ENGINE_H__
#define __RENDER_ENGINE_H__

#include "environment/scene/SceneFrame.h"

class RenderEngine {
  private:
    static RenderFrame sRenderFrame;
    static RayWithSegments *sPrimaryRay;
    static int sTraceLevel;

  public:
    static RenderFrame &renderFrame();
    static RayWithSegments *&primaryRay();
    static int &traceLevel();
    static double &maxTraceLevel();
    static volatile int &stopFlag();

    static void readRenderedPart(void);
    static void supersample(
        RGBAColor *result, int x, int y, int width, int height);
    static void startTracing(void);
    static void trace(RayWithSegments *ray, RGBAColor *color);
    static void initializeRenderer(void);
    static inline unsigned short rand3dInline(int a, int b);
};

#endif
