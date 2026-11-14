#include "environment/scene/NativeGeometryCrossingSink.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "environment/geometry/element/GeometryIntersectionEmissionContext.h"
#include "java/lang/UnsupportedOperationException.h"
#include "java/lang/Integer.h"
#include "java/util/PriorityQueue.txx"

NativeGeometryCrossingSink::NativeGeometryCrossingSink(Geometry* geometry,
    RayWithTracingState* ray, java::PriorityQueue<IntersectionCandidate>* queue,
    const GeometryIntersectionEmissionContext& context) :
    geometry(geometry), ray(ray), queue(queue), context(context), count(0) {}

void NativeGeometryCrossingSink::crossing(const RayCrossing& event)
{
    // The old candidate cannot represent a nested leaf, instance path or a
    // reversed boundary; reject these rather than silently losing provenance.
    if (event.geometry != geometry || event.normalReversed ||
        event.getInstanceCount() != 0 || event.surfaceIndex < -1 ||
        event.surfaceIndex > java::Integer::MAX_VALUE)
        throw java::UnsupportedOperationException("Nested crossing provenance is not representable");
    IntersectionCandidate candidate;
    candidate.getIntersection().t = event.t;
    candidate.getIntersection().point = ray->getDirection().multiply(event.t).add(ray->getOrigin());
    candidate.getAttributes().setHitSurface(geometry, SurfaceDetailFamily::Generic);
    candidate.getAttributes().setSurfaceIndex((int)event.surfaceIndex);
    candidate.getAttributes().setMaterial(context.getMaterialOverride());
    candidate.getAttributes().pushDetailOwner(context.getDetailOwner());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(context.getMaterialUsesObjectLocalPoint());
    queue->offer(candidate);
    ++count;
}
