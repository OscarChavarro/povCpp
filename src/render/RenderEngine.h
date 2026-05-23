#ifndef __RENDER_ENGINE_H__
#define __RENDER_ENGINE_H__

#include "environment/scene/SceneFrame.h"

class RenderEngine {
  public:
    static void readRenderedPart(void);
    static void supersample(
        RGBAColor *result, int x, int y, int width, int height);
    static void startTracing(void);
    static void trace(RayWithSegments *ray, RGBAColor *colour);
    static void initializeRenderer(void);
    static inline unsigned short rand3dInline(int a, int b);
};

#endif
