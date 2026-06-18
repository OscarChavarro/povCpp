#ifndef __TRACE_SERVICE__
#define __TRACE_SERVICE__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/element/RayWithSegments.h"

class TraceService {
  public:
    typedef void (*TraceFn)(void *context, RayWithSegments *ray, ColorRgba *color);
    typedef void (*ShadowShadeFn)(void *context, Intersection *intersection, ColorRgba *color);

    TraceService(TraceFn traceFn, ShadowShadeFn shadowShadeFn, void *context);
    TraceFn getTraceFn() const;
    void setTraceFn(TraceFn value);
    ShadowShadeFn getShadowShadeFn() const;
    void setShadowShadeFn(ShadowShadeFn value);
    void *getContext() const;
    void setContext(void *value);

    inline void trace(RayWithSegments *ray, ColorRgba *color) const
    {
        traceFn(context, ray, color);
    }

    inline void shadeShadow(Intersection *intersection, ColorRgba *color) const
    {
        shadowShadeFn(context, intersection, color);
    }

  private:
    TraceFn traceFn;
    ShadowShadeFn shadowShadeFn;
    void *context;
};

inline
TraceService::TraceService(TraceFn traceFn, ShadowShadeFn shadowShadeFn, void *context)
    : traceFn(traceFn), shadowShadeFn(shadowShadeFn), context(context)
{
}

inline TraceService::TraceFn
TraceService::getTraceFn() const
{
    return traceFn;
}

inline void
TraceService::setTraceFn(TraceFn value)
{
    traceFn = value;
}

inline TraceService::ShadowShadeFn
TraceService::getShadowShadeFn() const
{
    return shadowShadeFn;
}

inline void
TraceService::setShadowShadeFn(ShadowShadeFn value)
{
    shadowShadeFn = value;
}

inline void *
TraceService::getContext() const
{
    return context;
}

inline void
TraceService::setContext(void *value)
{
    context = value;
}

#endif
