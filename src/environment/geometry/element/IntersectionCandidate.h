#ifndef __INTERSECTION_CANDIDATE__
#define __INTERSECTION_CANDIDATE__

#include "vsdk/toolkit/environment/geometry/element/Intersection.h"
#include "environment/geometry/element/IntersectionAttributes.h"

// The unit travelling through the per-ray depth queues: the geometric record
// (Intersection) paired with its shading attribution (IntersectionAttributes).
// Kept as two explicit sub-objects, rather than flattened into Intersection
// itself, so Intersection stays a plain t/point/normal record; callers reach
// into whichever side they need via getIntersection()/getAttributes().
class IntersectionCandidate {
  private:
    Intersection intersection;
    IntersectionAttributes attributes;

  public:
    Intersection &getIntersection();
    const Intersection &getIntersection() const;
    IntersectionAttributes &getAttributes();
    const IntersectionAttributes &getAttributes() const;
};

inline Intersection &
IntersectionCandidate::getIntersection()
{
    return intersection;
}

inline const Intersection &
IntersectionCandidate::getIntersection() const
{
    return intersection;
}

inline IntersectionAttributes &
IntersectionCandidate::getAttributes()
{
    return attributes;
}

inline const IntersectionAttributes &
IntersectionCandidate::getAttributes() const
{
    return attributes;
}

template <>
inline bool
java::PriorityQueue<IntersectionCandidate>::lessThan(
    const IntersectionCandidate& a, const IntersectionCandidate& b) const
{
    return a.getIntersection().t < b.getIntersection().t;
}

#endif
