#ifndef __TRACE_SERVICE__
#define __TRACE_SERVICE__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/raySharedCache/RaySharedCache.h"

class TraceService {
  private:
    typedef void (*TraceFn)(void *context, const RayWithSegments *ray,
        ColorRgba *color);
    typedef void (*ShadowShadeFn)(void *context, IntersectionCandidate *intersection, ColorRgba *color);
    TraceFn traceFn;
    ShadowShadeFn shadowShadeFn;
    void *context;
    RaySharedCache *raySharedCache;

  public:
    TraceService(TraceFn traceFn, ShadowShadeFn shadowShadeFn, void *context,
        RaySharedCache *raySharedCache);
    inline void trace(const RayWithSegments *ray, ColorRgba *color) const;
    inline void shadeShadow(IntersectionCandidate *intersection, ColorRgba *color) const;
    RaySharedCache &getRaySharedCache() const { return *raySharedCache; }
};

inline
TraceService::TraceService(TraceFn traceFn, ShadowShadeFn shadowShadeFn,
    void *context, RaySharedCache *raySharedCache)
    : traceFn(traceFn), shadowShadeFn(shadowShadeFn), context(context),
      raySharedCache(raySharedCache)
{
}

inline void
TraceService::trace(const RayWithSegments *ray, ColorRgba *color) const
{
    traceFn(context, ray, color);
}

inline void
TraceService::shadeShadow(IntersectionCandidate *intersection, ColorRgba *color) const
{
    shadowShadeFn(context, intersection, color);
}

#endif
