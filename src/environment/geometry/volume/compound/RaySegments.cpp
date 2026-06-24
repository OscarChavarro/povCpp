#include <utility>
#include "java/util/ArrayList.txx"
#include "environment/geometry/volume/compound/RaySegments.h"

RaySegments::RaySegments(java::ArrayList<RaySegmentCrossing> crossings, bool initialInside) :
    crossings(std::move(crossings)),
    initialInside(initialInside)
{
}
