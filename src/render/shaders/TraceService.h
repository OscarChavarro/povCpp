#ifndef __TRACE_SERVICE_H__
#define __TRACE_SERVICE_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Intersection.h"
#include "environment/geometry/elements/RayWithSegments.h"

class TraceService {
public:
    typedef void (*TraceFn)(void *context, RayWithSegments *ray, ColorRgba *color);
    typedef void (*ShadowShadeFn)(void *context, Intersection *intersection, ColorRgba *color);

    TraceFn traceFn;
    ShadowShadeFn shadowShadeFn;
    void *context;

    inline void trace(RayWithSegments *ray, ColorRgba *color) const
    {
        traceFn(context, ray, color);
    }

    inline void shadeShadow(Intersection *intersection, ColorRgba *color) const
    {
        shadowShadeFn(context, intersection, color);
    }
};

#endif
