#ifndef VSDK_NATIVE_GEOMETRY_CROSSING_SINK_H
#define VSDK_NATIVE_GEOMETRY_CROSSING_SINK_H

#include "vsdk/toolkit/environment/geometry/element/RayCrossingSink.h"
#include "java/util/PriorityQueue.h"
#include "environment/geometry/element/IntersectionCandidate.h"

class NativeGeometryBinding;
class RayWithTracingState;
class GeometryIntersectionEmissionContext;

class NativeGeometryCrossingSink : public RayCrossingSink {
private:
    NativeGeometryBinding* binding;
    RayWithTracingState* ray;
    java::PriorityQueue<IntersectionCandidate>* queue;
    const GeometryIntersectionEmissionContext& context;
    int count;
public:
    NativeGeometryCrossingSink(NativeGeometryBinding* binding, RayWithTracingState* ray,
        java::PriorityQueue<IntersectionCandidate>* queue,
        const GeometryIntersectionEmissionContext& context);
    void crossing(const RayCrossing& event) override;
    int getCount() const { return count; }
};
#endif
