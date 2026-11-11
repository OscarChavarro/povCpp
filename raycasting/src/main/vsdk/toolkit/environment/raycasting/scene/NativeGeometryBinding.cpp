#include "java/lang/IllegalArgumentException.h"
#include "java/lang/IllegalStateException.h"
#include "java/lang/UnsupportedOperationException.h"
#include "java/util/PriorityQueue.txx"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/raycasting/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/raycasting/geometry/element/SphereTrace.h"
#include "vsdk/toolkit/environment/raycasting/scene/NativeGeometryBinding.h"
#include "vsdk/toolkit/environment/raycasting/scene/NativeGeometryCrossingSink.h"

NativeGeometryBinding::NativeGeometryBinding(Geometry* geometry, bool inverted) :
    geometry(geometry), sphere(dynamic_cast<const Sphere*>(geometry)), inverted(inverted)
{
    if (geometry == nullptr) throw java::IllegalArgumentException("Missing native geometry");
}

NativeGeometryBinding::NativeGeometryBinding(const NativeGeometryBinding& other) :
    geometry(other.geometry->copyGeometry()), sphere(dynamic_cast<const Sphere*>(geometry)),
    inverted(other.inverted)
{
    if (geometry == nullptr) throw java::UnsupportedOperationException("Geometry cannot be copied");
}

NativeGeometryBinding::~NativeGeometryBinding() { delete geometry; }
void* NativeGeometryBinding::copy() { return new NativeGeometryBinding(*this); }

bool NativeGeometryBinding::intersectSphere(const RayWithTracingState* ray,
    double* depth1, double* depth2) const
{
    return SphereTrace::intersect(*sphere, ray->getOrigin(), ray->getDirection(),
        ray->getGeometryStatistics(), depth1, depth2);
}

int NativeGeometryBinding::emitGeneric(RayWithTracingState* ray,
    java::PriorityQueue<IntersectionCandidate>* queue,
    const GeometryIntersectionEmissionContext& context)
{
    NativeGeometryCrossingSink sink(this, ray, queue, context);
    const Ray source = Ray::fromAffine(ray->getOrigin(), ray->getDirection());
    const RayCrossingQuery query(GeometryConfig::SMALL_TOLERANCE,
        GeometryConfig::MAX_DISTANCE, GeometryConfig::SMALL_TOLERANCE,
        GeometryConfig::SMALL_TOLERANCE);
    const RayCrossingSupport status = geometry->emitRayCrossings(source, query, sink);
    if (status == RayCrossingSupport::Unsupported)
        throw java::UnsupportedOperationException("Geometry does not support crossings");
    if (status != RayCrossingSupport::Supported)
        throw java::IllegalStateException("Geometry crossing query failed");
    return sink.getCount() > 0;
}

int NativeGeometryBinding::doIntersectionForAllRayCrossings(RayWithTracingState* ray,
    java::PriorityQueue<IntersectionCandidate>* queue, Material* material)
{
    return doIntersectionForAllRayCrossingsAnnotated(ray, queue,
        GeometryIntersectionEmissionContext(material, nullptr, false));
}

int NativeGeometryBinding::doIntersectionForAllRayCrossingsAnnotated(RayWithTracingState* ray,
    java::PriorityQueue<IntersectionCandidate>* queue,
    const GeometryIntersectionEmissionContext& context)
{
    if (sphere == nullptr) return emitGeneric(ray, queue, context);
    double depth1, depth2;
    if (!intersectSphere(ray, &depth1, &depth2)) return 0;
    IntersectionCandidate candidate;
    candidate.getAttributes().setHitGeometry(this);
    candidate.getAttributes().setMaterial(context.getMaterialOverride());
    candidate.getAttributes().pushDetailOwner(context.getDetailOwner());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(context.getMaterialUsesObjectLocalPoint());
    candidate.getIntersection().t = depth1;
    candidate.getIntersection().point = ray->getDirection().multiply(depth1).add(ray->getOrigin());
    queue->offer(candidate);
    if (depth2 != depth1) {
        candidate.getIntersection().t = depth2;
        candidate.getIntersection().point = ray->getDirection().multiply(depth2).add(ray->getOrigin());
        queue->offer(candidate);
    }
    return 1;
}

bool NativeGeometryBinding::doIntersectionFirstHit(RayWithTracingState* ray,
    IntersectionCandidate& out, Material* material)
{
    double depth;
    if (sphere != nullptr) {
        double depth1, depth2;
        if (!intersectSphere(ray, &depth1, &depth2)) return false;
        depth = depth1 < depth2 ? depth1 : depth2;
    } else {
        const Ray source = Ray::fromAffine(ray->getOrigin(), ray->getDirection());
        const RayCrossingQuery query(GeometryConfig::SMALL_TOLERANCE,
            GeometryConfig::MAX_DISTANCE, GeometryConfig::SMALL_TOLERANCE,
            GeometryConfig::SMALL_TOLERANCE);
        RayHit hit(RayHit::DETAIL_NONE, false);
        const RayFirstHitStatus status = geometry->queryFirstHit(source, query, &hit);
        if (status == RayFirstHitStatus::Miss) return false;
        if (status == RayFirstHitStatus::Unsupported)
            throw java::UnsupportedOperationException("Geometry does not support first hit");
        if (status != RayFirstHitStatus::Hit || !hit.hasHitDistance())
            throw java::IllegalStateException("Geometry first-hit query failed");
        depth = hit.getHitDistance();
    }
    out.getIntersection().t = depth;
    out.getIntersection().point = ray->getDirection().multiply(depth).add(ray->getOrigin());
    out.getAttributes().setHitGeometry(this);
    out.getAttributes().setMaterial(material);
    return true;
}

int NativeGeometryBinding::doContainmentTest(const Vector3Dd& point, double tolerance)
{
    if (sphere != nullptr) {
        const double implicit = sphere->implicitValue(point);
        return (inverted ? implicit > tolerance : implicit < tolerance) ? INSIDE : OUTSIDE;
    }
    const GeometryContainment result = geometry->classifyPoint(point, tolerance);
    if (result == GeometryContainment::Boundary) return LIMIT;
    if (result == GeometryContainment::Inside) return inverted ? OUTSIDE : INSIDE;
    if (result == GeometryContainment::Outside) return inverted ? INSIDE : OUTSIDE;
    if (result == GeometryContainment::Unsupported)
        throw java::UnsupportedOperationException("Geometry does not support containment");
    throw java::IllegalStateException("Geometry containment query failed");
}

void NativeGeometryBinding::doExtraInformation(const RayWithTracingState& ray,
    double t, RayCastingHit* hit)
{
    (void)t;
    if (!hit->needsNormal()) return;
    if (sphere != nullptr) {
        hit->n = sphere->surfaceNormalAtNonzeroPoint(hit->p);
        return;
    }
    const Ray local = Ray::fromAffine(hit->p, ray.getDirection());
    RayHit detail(RayHit::DETAIL_NORMAL, false);
    geometry->doExtraInformation(local, 0, &detail);
    if (!detail.isDetailAvailabilityKnown() || !(detail.getAvailableDetailMask() & RayHit::DETAIL_NORMAL))
        throw java::UnsupportedOperationException("Geometry did not provide a normal");
    hit->n = detail.normal;
}

BoundingVolumeHierarchy* NativeGeometryBinding::createBoundingVolume() const
{
    GeometryBounds bounds;
    geometry->getBounds(bounds);
    if (bounds.getKind() == GeometryBounds::Kind::Finite)
        return new AabbBoundingVolume(bounds.getMinimum(), bounds.getMaximum());
    return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
}
