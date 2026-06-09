#ifndef __TRACE_SERVICE_H__
#define __TRACE_SERVICE_H__

class RayWithSegments;
class ColorRgba;
class Intersection;

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
