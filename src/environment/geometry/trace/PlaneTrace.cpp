#include "environment/geometry/trace/PlaneTrace.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "java/util/PriorityQueue.txx"

bool
PlaneTrace::intersect(RayWithTracingState *ray, const PlanarHalfSpace &halfSpace,
    const SurfaceTraceState &state, double *depth)
{
    double normalDotOrigin;
    double normalDotDirection;
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    const InfinitePlane &plane = halfSpace.getBoundaryPlane();

    stats.incrementRayPlaneTests();
    if (ray->isPrimaryRayEnabled()) {
        if (!state.isViewpointCached()) {
            double value = plane.getCoefficients().dotProduct(ray->getOrigin());
            value += plane.getConstantCoefficient();
            value *= -1.0;
            state.setViewpointValue(value);
            state.setViewpointCached(true);
        }

        normalDotDirection = plane.getCoefficients().dotProduct(ray->getDirection());
        if (InfinitePlane::intersectionDepth(state.getViewpointValue(), normalDotDirection,
                GeometryConfig::SMALL_TOLERANCE, GeometryConfig::SMALL_TOLERANCE,
                GeometryConfig::MAX_DISTANCE, false, *depth) == RayFirstHitStatus::Hit) {
            stats.incrementRayPlaneTestsSucceeded();
            return true;
        }
        return false;
    }
    normalDotOrigin = plane.getCoefficients().dotProduct(ray->getOrigin());
    normalDotOrigin += plane.getConstantCoefficient();
    normalDotOrigin *= -1.0;

    normalDotDirection = plane.getCoefficients().dotProduct(ray->getDirection());
    if (InfinitePlane::intersectionDepth(normalDotOrigin, normalDotDirection,
            GeometryConfig::SMALL_TOLERANCE, GeometryConfig::SMALL_TOLERANCE,
            GeometryConfig::MAX_DISTANCE, false, *depth) == RayFirstHitStatus::Hit) {
        stats.incrementRayPlaneTestsSucceeded();
        return true;
    }
    return false;
}

int
PlaneTrace::emitCrossings(PlanarHalfSpace &halfSpace, const SurfaceTraceState &state,
    RayWithTracingState *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    double depth;
    Vector3Dd intersectionPoint;

    if (intersect(ray, halfSpace, state, &depth)) {
        if (depth > GeometryConfig::SMALL_TOLERANCE) {
            // Construct the candidate only on a real hit: the default ctor
            // zeroes ~168 bytes, wasted on the ~36% of plane tests that miss.
            IntersectionCandidate localElement;
            localElement.getIntersection().t = depth;
            intersectionPoint = ray->getDirection().multiply(depth);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().point = intersectionPoint;
            localElement.getAttributes().setHitSurface(&halfSpace, SurfaceDetailFamily::Plane);
            localElement.getAttributes().setMaterial(materialOverride);
            if (context != nullptr) {
                localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
                localElement.getAttributes().setMaterialUsesObjectLocalPoint(
                    context->getMaterialUsesObjectLocalPoint());
            }
            depthQueue->offer(localElement);
            return true;
        }
    }

    return false;
}

bool
PlaneTrace::firstHit(PlanarHalfSpace &halfSpace, const SurfaceTraceState &state,
    RayWithTracingState *ray, IntersectionCandidate &out, Material *materialOverride)
{
    double depth;
    if (!intersect(ray, halfSpace, state, &depth) || depth <= GeometryConfig::SMALL_TOLERANCE) {
        return false;
    }

    out.getIntersection().t = depth;
    out.getIntersection().point =
        ray->getDirection().multiply(depth).add(ray->getOrigin());
    out.getAttributes().setHitSurface(&halfSpace, SurfaceDetailFamily::Plane);
    out.getAttributes().setMaterial(materialOverride);
    return true;
}

int
PlaneTrace::containment(const PlanarHalfSpace &halfSpace, const Vector3Dd &point,
    double distanceTolerance)
{
    const InfinitePlane &plane = halfSpace.getBoundaryPlane();
    const double temp = point.dotProduct(plane.getCoefficients());
    return ((temp + plane.getConstantCoefficient()) <= distanceTolerance) ?
        static_cast<int>(Containment::INSIDE) : static_cast<int>(Containment::OUTSIDE);
}
