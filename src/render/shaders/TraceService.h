#ifndef __TRACE_SERVICE__
#define __TRACE_SERVICE__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"

class TraceService {
  private:
    typedef void (*TraceFn)(void *context, const RayWithSegments *ray,
        ColorRgba *color);
    typedef void (*ShadowShadeFn)(void *context, IntersectionCandidate *intersection, ColorRgba *color);
    TraceFn traceFn;
    ShadowShadeFn shadowShadeFn;
    void *context;

  public:
    TraceService(TraceFn traceFn, ShadowShadeFn shadowShadeFn, void *context);
    inline void trace(const RayWithSegments *ray, ColorRgba *color) const;
    inline void shadeShadow(IntersectionCandidate *intersection, ColorRgba *color) const;
};

inline
TraceService::TraceService(TraceFn traceFn, ShadowShadeFn shadowShadeFn,
    void *context)
    : traceFn(traceFn), shadowShadeFn(shadowShadeFn), context(context)
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
