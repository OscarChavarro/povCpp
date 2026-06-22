#ifndef __RENDER_WORKER__
#define __RENDER_WORKER__

#include "java/util/ArrayList.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/shaders/TraceService.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

class RenderEngine;

class RenderWorker {
  private:
    RenderEngine *engine;
    RayWithSegments ray;
    RayWithSegments *primaryRay;
    int traceLevel;
    ColorRgba *previousLine;
    ColorRgba *currentLine;
    char *previousLineAntiAliasedFlags;
    char *currentLineAntiAliasedFlags;
    TraceService traceService;
    TextureUtils *textureUtils;

    static ColorRgba *allocateColorBuffer(int count);
    static void traceServiceTrace(void *context, const RayWithSegments *ray,
        ColorRgba *color);
    static void traceServiceShadeShadow(
        void *context, IntersectionCandidate *intersection, ColorRgba *color);

  public:
    explicit RenderWorker(RenderEngine *owner);
    ~RenderWorker();

    void initializeLineBuffers(int width, bool antialiasEnabled);
    void swapLines();

    RayWithSegments *getPrimaryRay() { return primaryRay; }
    RayWithSegments &getRay() { return ray; }
    int &getTraceLevel() { return traceLevel; }
    void setTraceLevel(int level) { traceLevel = level; }
    ColorRgba *getPreviousLine() { return previousLine; }
    ColorRgba *getCurrentLine() { return currentLine; }
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
    TraceService *getTraceService() { return &traceService; }
    void setTextureUtils(TextureUtils *utils) { textureUtils = utils; }
    TextureUtils &getTextureUtils() { return *textureUtils; }
};

#endif
