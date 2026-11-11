#include "vsdk/toolkit/environment/raycasting/scene/NativeGeometryCrossingSink.h"
#include "vsdk/toolkit/environment/raycasting/scene/NativeGeometryBinding.h"
#include "java/lang/UnsupportedOperationException.h"
#include "java/util/PriorityQueue.txx"

NativeGeometryCrossingSink::NativeGeometryCrossingSink(NativeGeometryBinding* binding,
    RayWithTracingState* ray, java::PriorityQueue<IntersectionCandidate>* queue,
    const GeometryIntersectionEmissionContext& context) :
    binding(binding), ray(ray), queue(queue), context(context), count(0) {}

void NativeGeometryCrossingSink::crossing(const RayCrossing& event)
{
    // The old candidate cannot represent a nested leaf, instance path or a
    // reversed boundary; reject these rather than silently losing provenance.
    if (event.geometry != binding->getIntrinsicGeometry() || event.normalReversed ||
        event.getInstanceCount() != 0)
        throw java::UnsupportedOperationException("Nested crossing provenance is not representable");
    IntersectionCandidate candidate;
    candidate.getIntersection().t = event.t;
    candidate.getIntersection().point = ray->getDirection().multiply(event.t).add(ray->getOrigin());
    candidate.getAttributes().setHitGeometry(binding);
    candidate.getAttributes().setMaterial(context.getMaterialOverride());
    candidate.getAttributes().pushDetailOwner(context.getDetailOwner());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(context.getMaterialUsesObjectLocalPoint());
    queue->offer(candidate);
    ++count;
}
