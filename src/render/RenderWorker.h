#ifndef __RENDER_WORKER__
#define __RENDER_WORKER__

#include "java/util/ArrayList.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/shaders/TraceService.h"

class RenderEngine;

class RenderWorker {
  public:
    struct TraceEvent {
        bool childRay;
        RayWithSegments ray;
        ColorRgba color;
        TraceEvent() : childRay(false), color(0.0, 0.0, 0.0, 0.0) {}
    };

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
    java::ArrayList<TraceEvent*> *activeTraceEvents;

    static ColorRgba *allocateColorBuffer(int count);
    static void traceServiceTrace(void *context, const RayWithSegments *ray,
        const ColorRgba *multiplier);
    static void traceServiceAddColor(void *context, const ColorRgba *color);
    static void traceServiceShadeShadow(
        void *context, Intersection *intersection, ColorRgba *color);

  public:
    explicit RenderWorker(RenderEngine *owner);
    ~RenderWorker();

    void initializeLineBuffers(int width, bool antialiasEnabled);
    void swapLines();

    RayWithSegments *getPrimaryRay() { return primaryRay; }
    RayWithSegments &getRay() { return ray; }
    int getTraceLevel() const { return traceLevel; }
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
    java::ArrayList<TraceEvent*> *getActiveTraceEvents() { return activeTraceEvents; }
    void setActiveTraceEvents(java::ArrayList<TraceEvent*> *events) {
        activeTraceEvents = events;
    }
};

#endif
