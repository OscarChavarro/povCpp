#include "RayCrossingTestCollector.h"
#include "java/util/ArrayList.txx"

RayCrossingTestCollector::RayCrossingTestCollector() {}
RayCrossingTestCollector::~RayCrossingTestCollector() {}

void RayCrossingTestCollector::crossing(const RayCrossing& event)
{
    roots.add(event.t);
    kinds.add(event.kind);
    geometries.add(event.geometry);
    surfaceIndices.add((int)event.surfaceIndex);
    instanceCounts.add((int)event.getInstanceCount());
}
