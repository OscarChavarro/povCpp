#include "java/util/Collections.h"
#include "vsdk/toolkit/environment/raycasting/geometry/volume/constructiveSolidGeometry/RaySegments.h"
#include "java/util/ArrayList.txx"

RaySegments::RaySegments(java::ArrayList<RaySegmentCrossing> &crossingsSource, bool initialInside) :
    crossings(0),
    initialInside(initialInside)
{
    java::Collections::swap(crossings, crossingsSource);
}
