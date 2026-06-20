#ifndef __TRACE_SERVICE__
#define __TRACE_SERVICE__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/element/Intersection.h"
#include "environment/geometry/element/RayWithSegments.h"

class TraceService {
  private:
    typedef void (*TraceFn)(void *context, const RayWithSegments *ray,
        const ColorRgba *multiplier);
    typedef void (*AddColorFn)(void *context, const ColorRgba *color);
    typedef void (*ShadowShadeFn)(void *context, Intersection *intersection, ColorRgba *color);
    TraceFn traceFn;
    AddColorFn addColorFn;
    ShadowShadeFn shadowShadeFn;
    void *context;

  public:
    TraceService(TraceFn traceFn, AddColorFn addColorFn,
        ShadowShadeFn shadowShadeFn, void *context);
    inline void trace(const RayWithSegments *ray, const ColorRgba *multiplier) const;
    inline void addColor(const ColorRgba *color) const;
    inline void shadeShadow(Intersection *intersection, ColorRgba *color) const;
};

inline
TraceService::TraceService(TraceFn traceFn, AddColorFn addColorFn,
    ShadowShadeFn shadowShadeFn, void *context)
    : traceFn(traceFn), addColorFn(addColorFn),
      shadowShadeFn(shadowShadeFn), context(context)
{
}

inline void
TraceService::trace(const RayWithSegments *ray, const ColorRgba *multiplier) const
{
    traceFn(context, ray, multiplier);
}

inline void
TraceService::addColor(const ColorRgba *color) const
{
    addColorFn(context, color);
}

inline void
TraceService::shadeShadow(Intersection *intersection, ColorRgba *color) const
{
    shadowShadeFn(context, intersection, color);
}

#endif
