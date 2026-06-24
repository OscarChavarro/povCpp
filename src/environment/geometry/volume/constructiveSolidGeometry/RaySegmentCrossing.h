#ifndef __RAY_SEGMENT_CROSSING__
#define __RAY_SEGMENT_CROSSING__

#include "environment/geometry/element/IntersectionCandidate.h"

// One of RAYCAST's t[i]/S[i] ray-parameter/surface pairs ([ROTH1982].2.2,
// "Information from Ray Casting"), for a single CSG child.
class RaySegmentCrossing {
  private:
    double t;
    bool entering;
    IntersectionCandidate hit;

  public:
    RaySegmentCrossing();
    RaySegmentCrossing(double t, bool entering, const IntersectionCandidate &hit);

    double getT() const;
    bool isEntering() const;
    const IntersectionCandidate &getHit() const;
};

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
