#include "java/lang/IllegalStateException.h"
#include "java/lang/Integer.h"
#include "java/lang/UnsupportedOperationException.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RayCrossingQuery.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/element/SphereTrace.h"
#include "environment/scene/NativeSurfaceTrace.h"
#include "environment/scene/NativeGeometryCrossingSink.h"
#include "java/util/PriorityQueue.txx"

bool
NativeSurfaceTrace::intersectSphere(const Sphere &sphere, const RayWithTracingState *ray,
    double *depth1, double *depth2)
{
    return SphereTrace::intersect(sphere, ray->getOrigin(), ray->getDirection(),
        ray->getGeometryStatistics(), depth1, depth2);
}

int
NativeSurfaceTrace::emitSphereCrossings(Sphere &sphere, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *queue,
    const GeometryIntersectionEmissionContext &context)
{
    double depth1, depth2;
    if (!intersectSphere(sphere, ray, &depth1, &depth2)) return 0;
    IntersectionCandidate candidate;
    candidate.getAttributes().setHitSurface(&sphere, SurfaceDetailFamily::Sphere);
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

bool
NativeSurfaceTrace::sphereFirstHit(Sphere &sphere, RayWithTracingState *ray,
    IntersectionCandidate &out, Material *material)
{
    double depth1, depth2;
    if (!intersectSphere(sphere, ray, &depth1, &depth2)) return false;
    const double depth = depth1 < depth2 ? depth1 : depth2;
    out.getIntersection().t = depth;
    out.getIntersection().point = ray->getDirection().multiply(depth).add(ray->getOrigin());
    out.getAttributes().setHitSurface(&sphere, SurfaceDetailFamily::Sphere);
    out.getAttributes().setSurfaceIndex(-1);
    out.getAttributes().setMaterial(material);
    return true;
}

int
NativeSurfaceTrace::sphereContainment(const Sphere &sphere, const SurfaceTraceState &state,
    const Vector3Dd &point, double tolerance)
{
    const double implicit = sphere.implicitValue(point);
    return (state.isInverted() ? implicit > tolerance : implicit < tolerance) ?
        static_cast<int>(Containment::INSIDE) : static_cast<int>(Containment::OUTSIDE);
}

int
NativeSurfaceTrace::emitGenericCrossings(Geometry &geometry, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *queue,
    const GeometryIntersectionEmissionContext &context)
{
    NativeGeometryCrossingSink sink(&geometry, ray, queue, context);
    const Ray source = Ray::fromAffine(ray->getOrigin(), ray->getDirection());
    const RayCrossingQuery query(GeometryConfig::SMALL_TOLERANCE,
        GeometryConfig::MAX_DISTANCE);
    const RayCrossingSupport status = geometry.emitRayCrossings(source, query, sink);
    if (status == RayCrossingSupport::Unsupported)
        throw java::UnsupportedOperationException("Geometry does not support crossings");
    if (status != RayCrossingSupport::Supported)
        throw java::IllegalStateException("Geometry crossing query failed");
    return sink.getCount() > 0;
}

bool
NativeSurfaceTrace::genericFirstHit(Geometry &geometry, RayWithTracingState *ray,
    IntersectionCandidate &out, Material *material)
{
    const Ray source = Ray::fromAffine(ray->getOrigin(), ray->getDirection());
    const RayCrossingQuery query(GeometryConfig::SMALL_TOLERANCE,
        GeometryConfig::MAX_DISTANCE);
    RayHit hit(RayHit::DETAIL_NONE, false);
    RayBoundary boundary;
    const RayFirstHitStatus status = geometry.queryFirstHit(source, query, &hit, &boundary);
    if (status == RayFirstHitStatus::Miss) return false;
    if (status == RayFirstHitStatus::Unsupported)
        throw java::UnsupportedOperationException("Geometry does not support first hit");
    if (status != RayFirstHitStatus::Hit || !hit.hasHitDistance())
        throw java::IllegalStateException("Geometry first-hit query failed");
    if (boundary.geometry != &geometry || boundary.normalReversed || boundary.getInstanceCount() != 0 ||
        boundary.surfaceIndex < -1 || boundary.surfaceIndex > java::Integer::MAX_VALUE)
        throw java::UnsupportedOperationException("First-hit provenance is not representable");
    const double depth = hit.getHitDistance();
    out.getIntersection().t = depth;
    out.getIntersection().point = ray->getDirection().multiply(depth).add(ray->getOrigin());
    out.getAttributes().setHitSurface(&geometry, SurfaceDetailFamily::Generic);
    out.getAttributes().setSurfaceIndex((int)boundary.surfaceIndex);
    out.getAttributes().setMaterial(material);
    return true;
}

int
NativeSurfaceTrace::genericContainment(const Geometry &geometry, const SurfaceTraceState &state,
    const Vector3Dd &point, double tolerance)
{
    const int inside = static_cast<int>(Containment::INSIDE);
    const int outside = static_cast<int>(Containment::OUTSIDE);
    const GeometryContainment result = geometry.classifyPoint(point, tolerance);
    if (result == GeometryContainment::Boundary) return static_cast<int>(Containment::LIMIT);
    if (result == GeometryContainment::Inside) return state.isInverted() ? outside : inside;
    if (result == GeometryContainment::Outside) return state.isInverted() ? inside : outside;
    if (result == GeometryContainment::Unsupported)
        throw java::UnsupportedOperationException("Geometry does not support containment");
    throw java::IllegalStateException("Geometry containment query failed");
}
