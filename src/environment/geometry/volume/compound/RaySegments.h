#ifndef __RAY_SEGMENTS__
#define __RAY_SEGMENTS__

#include "java/util/ArrayList.h"
#include "environment/geometry/volume/compound/RaySegmentCrossing.h"

/**
 * A single child solid's full in/out classification along a ray: the
 * ordered list of boundary crossings, plus whether the ray was already
 * inside the child before the first crossing.
 *
 * This is RAYCAST's per-primitive classification result of
 * [ROTH1982].3.3 ("In-Out Classification") expressed as an explicit
 * sequence of crossings rather than the paper's N/PARMS/CLASSIFS triple;
 * initialInside records the in/out state of the segment before t[1], which
 * RAYCAST's CLASSIFS[1] would otherwise leave implicit.
 */
class RaySegments {
  private:
    java::ArrayList<RaySegmentCrossing> crossings;
    bool initialInside;

  public:
    // Taken by value (not const&): a caller passing an rvalue (every call
    // site in CSGByRaySegment.cpp passes std::move(localArray)) move-
    // constructs this parameter instead of copying it, and the member-
    // initializer below moves it again into `crossings` - two pointer-swap
    // moves instead of one `new T[n]` deep copy (see doc/CSGPerformance.md).
    RaySegments(java::ArrayList<RaySegmentCrossing> crossings, bool initialInside);

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
