#include "vsdk/toolkit/environment/scene/SceneSolid.h"
#include "environment/geometry/RayCastingGeometry.h"
#include "environment/geometry/SurfaceDetailKernel.h"
#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/RayCastingHit.h"
#include "java/util/PriorityQueue.txx"

void
RayCastingGeometry::applyAnnotatedEmissionContext(
    IntersectionCandidate &candidate,
    const GeometryIntersectionEmissionContext &context)
{
    IntersectionAttributes &attributes = candidate.getAttributes();
    attributes.pushDetailOwner(context.getDetailOwner());
    attributes.setMaterialUsesObjectLocalPoint(context.getMaterialUsesObjectLocalPoint());
}

int
RayCastingGeometry::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    if (depthQueue->size() == 0) {
        const int found =
            doIntersectionForAllRayCrossings(ray, depthQueue, context.getMaterialOverride());
        if (!found || depthQueue->size() == 0) {
            return found;
        }
        for (IntersectionCandidate &candidate : *depthQueue) {
            applyAnnotatedEmissionContext(candidate, context);
        }
        return found;
    }

    java::PriorityQueue<IntersectionCandidate> * const freshQueue =
        ray->getIntersectionQueuePool()->pop(128);
    const int found =
        doIntersectionForAllRayCrossings(ray, freshQueue, context.getMaterialOverride());
    if (found && freshQueue->size() > 0) {
        for (IntersectionCandidate &candidate : *freshQueue) {
            applyAnnotatedEmissionContext(candidate, context);
            depthQueue->offer(candidate);
        }
    }
    ray->getIntersectionQueuePool()->push(freshQueue);
    return found;
}

BoundingVolumeHierarchy *
RayCastingGeometry::createBoundingVolume() const
{
    return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
}

#include "environment/geometry/element/RayCastingQueryAdapter.h"

bool RayCastingGeometry::doIntersectionFirstHit(const Ray& source, RayHit* out)
{
    RayCastingQueryAdapter context(source);
    IntersectionCandidate candidate;
    bool found = doIntersectionFirstHit(&context.ray, candidate);
    if (!found) {
        auto* queue = context.pool.pop(128);
        doIntersectionForAllRayCrossings(&context.ray, queue);
        found = queue->size() > 0;
        if (found) candidate = queue->peek();
        context.pool.push(queue);
    }
    if (!found) return false;
    RayCastingHit detail = RayCastingHit::fromCandidate(candidate,
        out ? out->getRequiredDetailMask() : RayCastingHit::DETAIL_NONE);
    PostRayHitElement* owner = detail.popDetailOwnerBack();
    if (owner) owner->doExtraInformation(context.ray, detail.hitDistance, &detail);
    else SurfaceDetailKernel::resolve(context.ray, detail.hitDistance, &detail);
    RayCastingQueryAdapter::exportHit(source, detail, out);
    return true;
}

void RayCastingGeometry::doExtraInformation(const Ray& source, double t, RayHit* out)
{
    if (!out) return;
    RayCastingQueryAdapter context(source);
    IntersectionCandidate candidate;
    if (!context.findCrossing(this, t, candidate)) return;
    RayCastingHit detail = RayCastingHit::fromCandidate(candidate, out->getRequiredDetailMask());
    PostRayHitElement* owner = detail.popDetailOwnerBack();
    if (owner) owner->doExtraInformation(context.ray, t, &detail);
    else SurfaceDetailKernel::resolve(context.ray, t, &detail);
    RayCastingQueryAdapter::exportHit(source, detail, out);
}

double* RayCastingGeometry::getMinMax()
{
    BoundingVolumeHierarchy* volume = createBoundingVolume();
    const AxisAlignedBoundingBox bounds = volume->axisAlignedExtent();
    delete volume;
    return new double[6]{bounds.getMin().x(), bounds.getMin().y(), bounds.getMin().z(),
        bounds.getMax().x(), bounds.getMax().y(), bounds.getMax().z()};
}

RayCrossingSupport RayCastingGeometry::emitRayCrossings(const Ray& source,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    RayCastingQueryAdapter context(source);
    auto* queue = context.pool.pop(128);
    const_cast<RayCastingGeometry*>(this)->doIntersectionForAllRayCrossings(&context.ray, queue);
    while (queue->size() > 0) {
        const IntersectionCandidate candidate = queue->poll();
        const double t = candidate.getIntersection().t;
        if (t < query.minT || t > query.maxT) continue;
        RayCrossing event(t, candidate.getAttributes().getHitSurface(),
            RayCrossing::Kind::Unclassified);
        event.surfaceIndex = candidate.getAttributes().getSurfaceIndex();
        event.body = dynamic_cast<const SceneSolid*>(candidate.getAttributes().getHitBody());
        event.material = candidate.getAttributes().getMaterial();
        sink.crossing(event);
    }
    context.pool.push(queue);
    return RayCrossingSupport::Supported;
}
