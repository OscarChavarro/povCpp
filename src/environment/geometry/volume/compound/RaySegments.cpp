#include "java/util/ArrayList.txx"
#include "environment/geometry/volume/compound/RaySegments.h"

RaySegments::RaySegments(const java::ArrayList<RaySegmentCrossing> &crossings, bool initialInside) :
    crossings(crossings),
    initialInside(initialInside)
{
}

const java::ArrayList<RaySegmentCrossing> &
RaySegments::getCrossings() const
{
    return crossings;
}

bool
RaySegments::isInitialInside() const
{
    return initialInside;
}
