#ifndef __TRACE_SERVICE_H__
#define __TRACE_SERVICE_H__

class RayWithSegments;
class RGBAColor;
class Intersection;

class TraceService {
public:
    typedef void (*TraceFn)(void *context, RayWithSegments *ray, RGBAColor *colour);
    typedef void (*ShadowShadeFn)(void *context, Intersection *intersection, RGBAColor *colour);

    TraceFn traceFn;
    ShadowShadeFn shadowShadeFn;
    void *context;

    inline void trace(RayWithSegments *ray, RGBAColor *colour) const
    {
        traceFn(context, ray, colour);
    }

    inline void shadeShadow(Intersection *intersection, RGBAColor *colour) const
    {
        shadowShadeFn(context, intersection, colour);
    }
};

#endif
