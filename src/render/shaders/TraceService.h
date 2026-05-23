#ifndef __TRACE_SERVICE_H__
#define __TRACE_SERVICE_H__

class RayWithSegments;
class RGBAColor;
class Intersection;

class TraceService {
public:
    typedef void (*TraceFn)(void *context, RayWithSegments *ray, RGBAColor *color);
    typedef void (*ShadowShadeFn)(void *context, Intersection *intersection, RGBAColor *color);

    TraceFn traceFn;
    ShadowShadeFn shadowShadeFn;
    void *context;

    inline void trace(RayWithSegments *ray, RGBAColor *color) const
    {
        traceFn(context, ray, color);
    }

    inline void shadeShadow(Intersection *intersection, RGBAColor *color) const
    {
        shadowShadeFn(context, intersection, color);
    }
};

#endif
