#ifndef VSDK_GEOMETRY_QUERY_WORKSPACE_H
#define VSDK_GEOMETRY_QUERY_WORKSPACE_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"

// Caller-owned, reusable per worker/query; never share during concurrent queries.
class GeometryQueryWorkspace {
private:
    RaySolidClassification solid;
    GeometryBounds bounds;
    RayHit hit;
    java::ArrayList<RaySolidClassification*> scratch;
    long scratchDepth;
    GeometryQueryWorkspace(const GeometryQueryWorkspace&) = delete;
    GeometryQueryWorkspace& operator=(const GeometryQueryWorkspace&) = delete;
public:
    GeometryQueryWorkspace();
    ~GeometryQueryWorkspace();
    RaySolidClassification& solidResult();
    GeometryBounds& boundsResult();
    RayHit& hitResult();
    // Acquired pointers remain stable across pool growth; release in reverse order.
    RaySolidClassification* acquireSolidScratch();
    bool releaseSolidScratch(RaySolidClassification* value);
    void clear();
};
#endif
