#ifndef __RAY_CASTING_CONTAINMENT__
#define __RAY_CASTING_CONTAINMENT__

#include "vsdk/toolkit/processing/Containment.h"

// The integer containment codes of the POV render, from VITRAL `Containment`.
class RayCastingContainment {
  public:
    static constexpr int INSIDE = static_cast<int>(Containment::INSIDE);
    static constexpr int LIMIT = static_cast<int>(Containment::LIMIT);
    static constexpr int OUTSIDE = static_cast<int>(Containment::OUTSIDE);
};

#endif
