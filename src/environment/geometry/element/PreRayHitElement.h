#ifndef __PRE_RAY_HIT_ELEMENT__
#define __PRE_RAY_HIT_ELEMENT__

#include "environment/geometry/element/RayWithTracingState.h"

class PreRayHitElement {
  public:
    virtual ~PreRayHitElement() {};

    // Conservative filter for t in [0, +inf): false negatives are forbidden.
    virtual bool mayIntersect(const RayWithTracingState &ray) const = 0;

    // Conservative filter for t in [0, maxT]. The unbounded test is a valid
    // default because false positives are allowed.
    virtual bool mayIntersectBefore(const RayWithTracingState &ray, double maxT) const
    {
        (void)maxT;
        return mayIntersect(ray);
    }
};

#endif
