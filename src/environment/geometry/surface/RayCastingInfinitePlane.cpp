#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/surface/RayCastingInfinitePlane.h"
#include "environment/geometry/trace/PlaneTrace.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "java/util/PriorityQueue.txx"

RayCastingInfinitePlane::RayCastingInfinitePlane() :
    RayCastingInfinitePlane(Vector3Dd(0.0, 1.0, 0.0), 0.0)
{
}

RayCastingInfinitePlane::RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance) :
    RayCastingInfinitePlane(normalVector, distance, 0.0, false)
{
}

RayCastingInfinitePlane::RayCastingInfinitePlane(const Vector3Dd &normalVector, double distance,
    double vpNormDotOrigin, bool vpCached) :
    halfSpace(normalVector.x(), normalVector.y(), normalVector.z(), distance)
{
    traceState.setViewpointValue(vpNormDotOrigin);
    traceState.setViewpointCached(vpCached);
}

void RayCastingInfinitePlane::getBounds(GeometryBounds& result) const
{
    halfSpace.getBounds(result);
}

GeometryContainment RayCastingInfinitePlane::classifyPoint(const Vector3Dd& point,
    double tolerance) const
{
    return halfSpace.classifyPoint(point, tolerance);
}

RayCrossingSupport RayCastingInfinitePlane::emitRayCrossings(const Ray& ray,
    const RayCrossingQuery& query, RayCrossingSink& sink) const
{
    return halfSpace.emitRayCrossings(ray, query, sink);
}

RayFirstHitStatus RayCastingInfinitePlane::queryFirstHit(const Ray& ray,
    const RayCrossingQuery& query, RayHit* hit, RayBoundary* boundary) const
{
    return halfSpace.queryFirstHit(ray, query, hit, boundary);
}

RaySolidQueryStatus RayCastingInfinitePlane::classifyRay(const Ray& ray,
    const RayCrossingQuery& query, RaySolidClassification& result) const
{
    return halfSpace.classifyRay(ray, query, result);
}

int
RayCastingInfinitePlane::doIntersectionForAllRayCrossings(
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    return PlaneTrace::emitCrossings(halfSpace, traceState, ray, depthQueue, materialOverride,
        nullptr);
}

bool
RayCastingInfinitePlane::doIntersectionFirstHit(
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    return PlaneTrace::firstHit(halfSpace, traceState, ray, out, materialOverride);
}

int
RayCastingInfinitePlane::doContainmentTest(const Vector3Dd &testPoint, double distanceTolerance)
{
    return PlaneTrace::containment(halfSpace, testPoint, distanceTolerance);
}

void *
RayCastingInfinitePlane::copy()
{
    return new RayCastingInfinitePlane(*this);
}

void
RayCastingInfinitePlane::invertGeometry()
{
    RayCastingInfinitePlane * const plane = this;

    const Vector3Dd normal = plane->getNormalVector().multiply(-1.0);
    const double distance = -plane->getDistance();
    plane->halfSpace.getBoundaryPlane().setCoefficients(normal, distance);
    plane->traceState.setViewpointCached(false);
}
