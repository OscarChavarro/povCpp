#include "java/lang/IllegalArgumentException.h"
#include "java/lang/IllegalStateException.h"
#include "java/lang/Integer.h"
#include "java/lang/UnsupportedOperationException.h"
#include "java/util/PriorityQueue.txx"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RayBoundary.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/geometry/volume/Sphere.h"
#include "vsdk/toolkit/environment/geometry/surface/TriangleMesh.h"
#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/element/SphereTrace.h"
#include "environment/scene/NativeGeometryBinding.h"
#include "environment/scene/NativeGeometryCrossingSink.h"
#include "environment/scene/ProjectedTriangleTraversal.h"
#include "environment/geometry/volume/BoxInvertedCrossingSink.h"

NativeGeometryBinding::NativeGeometryBinding(Geometry* geometry, bool inverted) :
    geometry(geometry), sphere(dynamic_cast<const Sphere*>(geometry)), triangleTraversal(nullptr), inverted(inverted)
{
    if (geometry == nullptr) throw java::IllegalArgumentException("Missing native geometry");
}

NativeGeometryBinding::NativeGeometryBinding(const NativeGeometryBinding& other) :
    geometry(other.geometry->copyGeometry()), sphere(dynamic_cast<const Sphere*>(geometry)),
    triangleTraversal(nullptr), inverted(other.inverted)
{
    if (geometry == nullptr) throw java::UnsupportedOperationException("Geometry cannot be copied");
    if (other.triangleTraversal != nullptr) {
        TriangleMesh* mesh = dynamic_cast<TriangleMesh*>(geometry);
        if (mesh == nullptr) { delete geometry; throw java::IllegalStateException("Copied triangle geometry is invalid"); }
        triangleTraversal = new ProjectedTriangleTraversal(*other.triangleTraversal, mesh);
    }
}

NativeGeometryBinding::~NativeGeometryBinding() { delete triangleTraversal; delete geometry; }
void* NativeGeometryBinding::copy() { return new NativeGeometryBinding(*this); }

RayCrossingSupport NativeGeometryBinding::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    if (triangleTraversal != nullptr)
        return RayCastingGeometry::emitRayCrossings(ray, query, sink);
    if (!inverted) return geometry->emitRayCrossings(ray, query, sink);
    BoxInvertedCrossingSink reversed(sink);
    return geometry->emitRayCrossings(ray, query, reversed);
}

GeometryContainment NativeGeometryBinding::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    if (triangleTraversal != nullptr) return GeometryContainment::Unsupported;
    const GeometryContainment result = geometry->classifyPoint(point, tolerance);
    if (!inverted) return result;
    if (result == GeometryContainment::Inside) return GeometryContainment::Outside;
    if (result == GeometryContainment::Outside) return GeometryContainment::Inside;
    return result;
}

RaySolidQueryStatus NativeGeometryBinding::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    if (triangleTraversal != nullptr) { result.clear(); return RaySolidQueryStatus::Unsupported; }
    const RaySolidQueryStatus status = geometry->classifyRay(ray, query, result);
    if (status == RaySolidQueryStatus::Supported && inverted) {
        for (long int i=0; i<result.insideSegments.size(); ++i)
            result.insideSegments[i] = !result.insideSegments[i];
        for (long int i=0; i<result.boundaries.size(); ++i)
            result.boundaries[i].normalReversed = !result.boundaries[i].normalReversed;
    }
    return status;
}

void NativeGeometryBinding::enableProjectedTriangleTraversal()
{
    TriangleMesh* mesh = dynamic_cast<TriangleMesh*>(geometry);
    if (mesh == nullptr || triangleTraversal != nullptr || inverted)
        throw java::IllegalStateException("Projected traversal needs a valid, non-inverted triangle mesh");
    triangleTraversal = new ProjectedTriangleTraversal(mesh);
}

void NativeGeometryBinding::invertGeometry()
{
    if (triangleTraversal != nullptr) triangleTraversal->invert();
    else inverted = !inverted;
}

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
        GeometryConfig::MAX_DISTANCE);
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
    if (triangleTraversal != nullptr) return triangleTraversal->emit(this, ray, queue, context);
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
    if (triangleTraversal != nullptr) return false;
    double depth;
    int surfaceIndex = -1;
    if (sphere != nullptr) {
        double depth1, depth2;
        if (!intersectSphere(ray, &depth1, &depth2)) return false;
        depth = depth1 < depth2 ? depth1 : depth2;
    } else {
        const Ray source = Ray::fromAffine(ray->getOrigin(), ray->getDirection());
        const RayCrossingQuery query(GeometryConfig::SMALL_TOLERANCE,
            GeometryConfig::MAX_DISTANCE);
        RayHit hit(RayHit::DETAIL_NONE, false);
        RayBoundary boundary;
        const RayFirstHitStatus status = geometry->queryFirstHit(source, query, &hit, &boundary);
        if (status == RayFirstHitStatus::Miss) return false;
        if (status == RayFirstHitStatus::Unsupported)
            throw java::UnsupportedOperationException("Geometry does not support first hit");
        if (status != RayFirstHitStatus::Hit || !hit.hasHitDistance())
            throw java::IllegalStateException("Geometry first-hit query failed");
        if (boundary.geometry != geometry || boundary.normalReversed || boundary.getInstanceCount() != 0 ||
            boundary.surfaceIndex < -1 || boundary.surfaceIndex > java::Integer::MAX_VALUE)
            throw java::UnsupportedOperationException("First-hit provenance is not representable");
        surfaceIndex = (int)boundary.surfaceIndex;
        depth = hit.getHitDistance();
    }
    out.getIntersection().t = depth;
    out.getIntersection().point = ray->getDirection().multiply(depth).add(ray->getOrigin());
    out.getAttributes().setHitGeometry(this);
    out.getAttributes().setSurfaceIndex(surfaceIndex);
    out.getAttributes().setMaterial(material);
    return true;
}

int NativeGeometryBinding::doContainmentTest(const Vector3Dd& point, double tolerance)
{
    if (triangleTraversal != nullptr) return OUTSIDE;
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
    if (triangleTraversal != nullptr) { triangleTraversal->doExtraInformation(hit); return; }
    if (!hit->needsNormal()) return;
    if (sphere != nullptr) {
        hit->n = sphere->surfaceNormalAtNonzeroPoint(hit->p);
        return;
    }
    const Ray local = Ray::fromAffine(ray.getOrigin(), ray.getDirection());
    RayHit detail(RayHit::DETAIL_NORMAL, false);
    if (hit->surfaceIndex >= 0) {
        const RayBoundary boundary(t, geometry, false, hit->surfaceIndex);
        if (geometry->querySurfaceDetail(local, boundary, &detail) != RayFirstHitStatus::Hit)
            throw java::UnsupportedOperationException("Geometry did not resolve the selected surface");
    } else {
        const Ray pointRay = Ray::fromAffine(hit->p, ray.getDirection());
        geometry->doExtraInformation(pointRay, 0, &detail);
    }
    if (!detail.isDetailAvailabilityKnown() || !(detail.getAvailableDetailMask() & RayHit::DETAIL_NORMAL))
        throw java::UnsupportedOperationException("Geometry did not provide a normal");
    hit->n = detail.normal;
}

void NativeGeometryBinding::getBounds(GeometryBounds& result) const
{
    // A complemented solid or a projected inverted triangle can extend to
    // infinity, regardless of the bounds of the underlying finite geometry.
    if (inverted || (triangleTraversal != nullptr && triangleTraversal->hasUnboundedFaces())) {
        result.setUnbounded();
        return;
    }
    geometry->getBounds(result);
}

BoundingVolumeHierarchy* NativeGeometryBinding::createBoundingVolume() const
{
    if (triangleTraversal != nullptr && triangleTraversal->hasUnboundedFaces())
        return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
    GeometryBounds bounds;
    geometry->getBounds(bounds);
    if (bounds.getKind() == GeometryBounds::Kind::Finite)
        return new AabbBoundingVolume(bounds.getMinimum(), bounds.getMaximum());
    return new AabbBoundingVolume(AxisAlignedBoundingBox::unbounded());
}
