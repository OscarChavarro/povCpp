#ifndef __POST_RAY_HIT_ELEMENT__
#define __POST_RAY_HIT_ELEMENT__

#include "environment/geometry/element/RayWithTracingState.h"

class PovRayHit;

class PostRayHitElement {
  public:
    virtual ~PostRayHitElement() {};
    virtual void doExtraInformation(
        const RayWithTracingState &ray, double t, PovRayHit *hit) = 0;
};

#endif
