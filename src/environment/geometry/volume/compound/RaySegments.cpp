#include "java/util/ArrayList.txx"
#include "java/util/Collections.h"
#include "environment/geometry/volume/compound/RaySegments.h"

RaySegments::RaySegments(java::ArrayList<RaySegmentCrossing> &crossingsSource, bool initialInside) :
    crossings(0),
    initialInside(initialInside)
{
    java::Collections::swap(crossings, crossingsSource);
}
