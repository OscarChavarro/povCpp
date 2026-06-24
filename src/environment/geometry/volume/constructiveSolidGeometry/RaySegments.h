#ifndef __RAY_SEGMENTS__
#define __RAY_SEGMENTS__

#include "java/util/ArrayList.h"
#include "environment/geometry/volume/constructiveSolidGeometry/RaySegmentCrossing.h"

// RAYCAST's per-primitive in/out classification result ([ROTH1982].3.3).
class RaySegments {
  private:
    java::ArrayList<RaySegmentCrossing> crossings;
    bool initialInside;

  public:
    RaySegments(java::ArrayList<RaySegmentCrossing> &crossingsSource, bool initialInside);

    const java::ArrayList<RaySegmentCrossing> &getCrossings() const;
    bool isInitialInside() const;
};

inline const java::ArrayList<RaySegmentCrossing> &
RaySegments::getCrossings() const
{
    return crossings;
}

inline bool
RaySegments::isInitialInside() const
{
    return initialInside;
}

#endif
