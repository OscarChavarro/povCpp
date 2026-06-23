#include "environment/geometry/volume/compound/RaySegmentCrossing.h"

RaySegmentCrossing::RaySegmentCrossing() :
    t(0.0),
    entering(false)
{
}

RaySegmentCrossing::RaySegmentCrossing(double t, bool entering, const IntersectionCandidate &hit) :
    t(t),
    entering(entering),
    hit(hit)
{
}

double
RaySegmentCrossing::getT() const
{
    return t;
}

bool
RaySegmentCrossing::isEntering() const
{
    return entering;
}

const IntersectionCandidate &
RaySegmentCrossing::getHit() const
{
    return hit;
}
