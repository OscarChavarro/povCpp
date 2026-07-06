#ifndef __RAY_CASTING_HIT_ELEMENT__
#define __RAY_CASTING_HIT_ELEMENT__

#include "environment/geometry/element/RayWithTracingState.h"

class PovRayHit;

class RayCastingHitElement {
  public:
    virtual ~RayCastingHitElement() {};
    virtual void doExtraInformation(
        const RayWithTracingState &ray, double t, PovRayHit *hit) = 0;
};

#endif
