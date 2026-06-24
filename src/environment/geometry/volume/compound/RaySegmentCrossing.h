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

// Defined here (not in the .cpp) so the optimizer can actually inline them at
// every one of the hundreds of millions of call sites CSGByRaySegment makes
// per render (see doc/CSGPerformance.md): a definition living in a separate
// translation unit can never be inlined into its callers without LTO,
// regardless of optimization level or the `inline` keyword on the .cpp
// definition - the keyword only relaxes the one-definition rule, it does not
// make the body visible to the caller's compilation.
inline
RaySegmentCrossing::RaySegmentCrossing() :
    t(0.0),
    entering(false)
{
}

inline
RaySegmentCrossing::RaySegmentCrossing(double t, bool entering, const IntersectionCandidate &hit) :
    t(t),
    entering(entering),
    hit(hit)
{
}

inline double
RaySegmentCrossing::getT() const
{
    return t;
}

inline bool
RaySegmentCrossing::isEntering() const
{
    return entering;
}

inline const IntersectionCandidate &
RaySegmentCrossing::getHit() const
{
    return hit;
}

#endif
