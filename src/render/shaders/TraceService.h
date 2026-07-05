#ifndef __TRACE_SERVICE__
#define __TRACE_SERVICE__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "render/raySharedCache/RaySharedCache.h"

class PovRayRenderStatistics;

// Carries the calling task's own full PovRayRenderStatistics (see RenderEngine::renderTile),
// separately from RayWithTracingState (which only carries GeometryStatistics).
// Shaders that need shading-level counters (reflected/refracted/transmitted rays,
// shadow ray tests, number of rays) reach them here instead of through the ray,
// since TraceService already threads through the whole shader call chain.
class TraceService {
  private:
    typedef void (*TraceFn)(void *context, const RayWithTracingState *ray,
        ColorRgba *color);
    typedef void (*ShadowShadeFn)(void *context, IntersectionCandidate *intersection, ColorRgba *color);
    TraceFn traceFn;
    ShadowShadeFn shadowShadeFn;
    void *context;
    RaySharedCache *raySharedCache;
    PovRayRenderStatistics *statistics;

  public:
    TraceService(TraceFn traceFn, ShadowShadeFn shadowShadeFn, void *context,
        RaySharedCache *raySharedCache);
    inline void trace(const RayWithTracingState *ray, ColorRgba *color) const;
    inline void shadeShadow(IntersectionCandidate *intersection, ColorRgba *color) const;
    RaySharedCache &getRaySharedCache() const { return *raySharedCache; }
    void setStatistics(PovRayRenderStatistics *stats) { statistics = stats; }
    PovRayRenderStatistics *getStatistics() const { return statistics; }
};

inline
TraceService::TraceService(TraceFn traceFn, ShadowShadeFn shadowShadeFn,
    void *context, RaySharedCache *raySharedCache)
    : traceFn(traceFn), shadowShadeFn(shadowShadeFn), context(context),
      raySharedCache(raySharedCache), statistics(nullptr)
{
}

inline void
TraceService::trace(const RayWithTracingState *ray, ColorRgba *color) const
{
    traceFn(context, ray, color);
}

inline void
TraceService::shadeShadow(IntersectionCandidate *intersection, ColorRgba *color) const
{
    shadowShadeFn(context, intersection, color);
}

#endif
