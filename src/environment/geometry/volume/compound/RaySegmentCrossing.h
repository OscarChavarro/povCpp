#ifndef __RAY_SEGMENT_CROSSING__
#define __RAY_SEGMENT_CROSSING__

#include "environment/geometry/element/IntersectionCandidate.h"

/**
 * A single crossing of a ray with a child solid's boundary: the ray
 * parameter at the hit, the side the ray moves to at that point, and the
 * full intersection record needed to shade the hit if it survives the
 * boolean classification.
 *
 * Corresponds to one element of RAYCAST's returned ray-parameter/surface-
 * pointer lists t[i]/S[i] in [ROTH1982].2.2 ("Information from Ray
 * Casting"), specialized here to a single child of a CSG node rather than
 * the whole composition tree.
 */
class RaySegmentCrossing {
  private:
    double t;
    bool entering;
    IntersectionCandidate hit;

  public:
    // java::ArrayList<T> allocates with `new T[n]`, which requires T to be
    // default-constructible; this constructor exists only to satisfy that
    // container requirement. Every crossing actually used by CSGByRaySegment
    // is built through the constructor below.
    RaySegmentCrossing();
    RaySegmentCrossing(double t, bool entering, const IntersectionCandidate &hit);

    double getT() const;
    bool isEntering() const;
    const IntersectionCandidate &getHit() const;
};

#endif
