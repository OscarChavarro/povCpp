#include "environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"
#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/volume/RayCastingBox.h"
#include "environment/geometry/trace/BoxTrace.h"
#include "vsdk/toolkit/environment/geometry/volume/Box.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "environment/geometry/volume/BoxInvertedCrossingSink.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

RayCastingBox::RayCastingBox() :
    RayCastingBox(Vector3Dd(-1.0, -1.0, -1.0), Vector3Dd(1.0, 1.0, 1.0), false)
{
}

RayCastingBox::RayCastingBox(const Vector3Dd &minBounds, const Vector3Dd &maxBounds, bool inverted) :
    box(minBounds, maxBounds)
{
    traceState.setInverted(inverted);
}

RayCastingBox::RayCastingBox(const RayCastingBox &other) :
    RayCastingBox(other.getBounds()[0], other.getBounds()[1], other.isInverted())
{
}

RayCastingBox::~RayCastingBox()
{
}

int
RayCastingBox::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return BoxTrace::emitCrossings(box, ray, depthQueue, materialOverride, nullptr);
}

int
RayCastingBox::doIntersectionForAllRayCrossingsAnnotated(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    const GeometryIntersectionEmissionContext &context)
{
    return BoxTrace::emitCrossings(box, ray, depthQueue, context.getMaterialOverride(), &context);
}

bool
RayCastingBox::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    return BoxTrace::firstHit(box, ray, out, materialOverride);
}

int
RayCastingBox::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    return BoxTrace::containment(box, traceState, testPoint, distanceTolerance);
}

void *
RayCastingBox::copy()
{
    return new RayCastingBox(*this);
}

void
RayCastingBox::invertGeometry()
{
    traceState.setInverted(!traceState.isInverted());
}

BoundingVolumeHierarchy *
RayCastingBox::createBoundingVolume() const
{
    return new AabbBoundingVolume(box.getRawBounds()[0], box.getRawBounds()[1]);
}

void RayCastingBox::getBounds(GeometryBounds& result) const
{
    if (traceState.isInverted()) result.setUnbounded();
    else box.getBounds(result);
}

GeometryContainment RayCastingBox::classifyPoint(const Vector3Dd& point, double tolerance) const
{
    const GeometryContainment result = box.classifyPoint(point, tolerance);
    if (!traceState.isInverted()) return result;
    if (result == GeometryContainment::Inside) return GeometryContainment::Outside;
    if (result == GeometryContainment::Outside) return GeometryContainment::Inside;
    return result;
}
RayCrossingSupport RayCastingBox::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    if (!traceState.isInverted()) return box.emitRayCrossings(ray, query, sink);
    BoxInvertedCrossingSink reversed(sink);
    return box.emitRayCrossings(ray, query, reversed);
}
RayFirstHitStatus RayCastingBox::queryFirstHit(const Ray& ray, const RayCrossingQuery& query,
    RayHit* hit, RayBoundary* boundary) const
{
    const RayFirstHitStatus status = box.queryFirstHit(ray, query, hit, boundary);
    if (status == RayFirstHitStatus::Hit && traceState.isInverted()) {
        if (boundary) boundary->normalReversed = !boundary->normalReversed;
        if (hit && hit->needsNormal() && (hit->getAvailableDetailMask() & RayHit::DETAIL_NORMAL))
            hit->normal = hit->normal.multiply(-1);
    }
    return status;
}
RayFirstHitStatus RayCastingBox::querySurfaceDetail(const Ray& ray,
    const RayBoundary& boundary, RayHit* hit) const
{
    return box.querySurfaceDetail(ray, boundary, hit);
}
RaySolidQueryStatus RayCastingBox::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    const RaySolidQueryStatus status = box.classifyRay(ray, query, result);
    if (status == RaySolidQueryStatus::Supported && traceState.isInverted()) {
        for (long int i=0; i<result.insideSegments.size(); ++i)
            result.insideSegments[i] = !result.insideSegments[i];
        for (long int i=0; i<result.boundaries.size(); ++i)
            result.boundaries[i].normalReversed = !result.boundaries[i].normalReversed;
    }
    return status;
}
