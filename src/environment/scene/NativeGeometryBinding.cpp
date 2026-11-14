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
#include "environment/scene/NativeGeometryBinding.h"
#include "environment/scene/SurfaceTraceKernel.h"
#include "environment/scene/ProjectedTriangleTraversal.h"
#include "environment/geometry/volume/BoxInvertedCrossingSink.h"

NativeGeometryBinding::NativeGeometryBinding(Geometry* geometry, bool inverted) :
    geometry(geometry), sphere(dynamic_cast<const Sphere*>(geometry)), triangleTraversal(nullptr)
{
    if (geometry == nullptr) throw java::IllegalArgumentException("Missing native geometry");
    traceState.setInverted(inverted);
}

NativeGeometryBinding::NativeGeometryBinding(const NativeGeometryBinding& other) :
    geometry(other.geometry->copyGeometry()), sphere(dynamic_cast<const Sphere*>(geometry)),
    triangleTraversal(nullptr)
{
    if (geometry == nullptr) throw java::UnsupportedOperationException("Geometry cannot be copied");
    traceState.setInverted(other.traceState.isInverted());
    if (other.triangleTraversal != nullptr) {
        TriangleMesh* mesh = dynamic_cast<TriangleMesh*>(geometry);
        if (mesh == nullptr) { delete geometry; throw java::IllegalStateException("Copied triangle geometry is invalid"); }
        triangleTraversal = new ProjectedTriangleTraversal(*other.triangleTraversal, mesh);
        traceState.setTriangleTraversal(triangleTraversal);
    }
}

NativeGeometryBinding::~NativeGeometryBinding() { delete triangleTraversal; delete geometry; }
void* NativeGeometryBinding::copy() { return new NativeGeometryBinding(*this); }

RayCrossingSupport NativeGeometryBinding::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    if (triangleTraversal != nullptr)
        return RayCastingGeometry::emitRayCrossings(ray, query, sink);
    if (!traceState.isInverted()) return geometry->emitRayCrossings(ray, query, sink);
    BoxInvertedCrossingSink reversed(sink);
    return geometry->emitRayCrossings(ray, query, reversed);
}

GeometryContainment NativeGeometryBinding::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    if (triangleTraversal != nullptr) return GeometryContainment::Unsupported;
    const GeometryContainment result = geometry->classifyPoint(point, tolerance);
    if (!traceState.isInverted()) return result;
    if (result == GeometryContainment::Inside) return GeometryContainment::Outside;
    if (result == GeometryContainment::Outside) return GeometryContainment::Inside;
    return result;
}

RaySolidQueryStatus NativeGeometryBinding::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    if (triangleTraversal != nullptr) { result.clear(); return RaySolidQueryStatus::Unsupported; }
    const RaySolidQueryStatus status = geometry->classifyRay(ray, query, result);
    if (status == RaySolidQueryStatus::Supported && traceState.isInverted()) {
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
    if (mesh == nullptr || triangleTraversal != nullptr || traceState.isInverted())
        throw java::IllegalStateException("Projected traversal needs a valid, non-inverted triangle mesh");
    triangleTraversal = new ProjectedTriangleTraversal(mesh);
    traceState.setTriangleTraversal(triangleTraversal);
}

void NativeGeometryBinding::invertGeometry()
{
    if (triangleTraversal != nullptr) triangleTraversal->invert();
    else traceState.setInverted(!traceState.isInverted());
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
    return SurfaceTraceKernel::emitCrossings(geometry, NativeGeometryBinding::getDetailFamily(), traceState, ray, queue,
        context.getMaterialOverride(), &context);
}

bool NativeGeometryBinding::doIntersectionFirstHit(RayWithTracingState* ray,
    IntersectionCandidate& out, Material* material)
{
    return SurfaceTraceKernel::firstHit(geometry, NativeGeometryBinding::getDetailFamily(), traceState, ray, out, material);
}

int NativeGeometryBinding::doContainmentTest(const Vector3Dd& point, double tolerance)
{
    return SurfaceTraceKernel::containment(geometry, NativeGeometryBinding::getDetailFamily(), traceState, point, tolerance);
}

SurfaceDetailFamily NativeGeometryBinding::getDetailFamily() const
{
    if (triangleTraversal != nullptr) return SurfaceDetailFamily::Mesh;
    return sphere != nullptr ? SurfaceDetailFamily::Sphere : SurfaceDetailFamily::Generic;
}

void NativeGeometryBinding::getBounds(GeometryBounds& result) const
{
    // A complemented solid or a projected inverted triangle can extend to
    // infinity, regardless of the bounds of the underlying finite geometry.
    if (traceState.isInverted() || (triangleTraversal != nullptr && triangleTraversal->hasUnboundedFaces())) {
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
