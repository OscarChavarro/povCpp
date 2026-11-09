#ifndef __POST_RAY_HIT_ELEMENT__
#define __POST_RAY_HIT_ELEMENT__

#include "vsdk/toolkit/environment/raycasting/geometry/element/RayWithTracingState.h"

class RayCastingHit;

class PostRayHitElement {
  public:
    virtual ~PostRayHitElement() {};
    virtual void doExtraInformation(
        const RayWithTracingState &ray, double t, RayCastingHit *hit) = 0;
};

#endif
