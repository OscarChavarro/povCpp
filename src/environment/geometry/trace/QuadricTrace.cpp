#include "environment/geometry/trace/QuadricTrace.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/geometry/volume/QuadricRayExpansion.h"
#include "java/util/PriorityQueue.txx"

bool
QuadricTrace::intersect(RayWithTracingState *ray, const QuadricSurface &surface,
    const SurfaceTraceState &state, double *depth1, double *depth2)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();

    stats.incrementRayQuadricTests();
    if (!ray->areQuadricConstantsCached()) {
        ray->makeRay();
    }

    const double squareTerm = QuadricRayExpansion::squareTerm(surface, state.hasNonZeroSquareTerm(),
        ray->getDirection2(), ray->getMixedDirectionDirection());
    const double linearTerm = QuadricRayExpansion::linearTerm(surface, ray->getPositionDirection(),
        ray->getDirection(), ray->getMixedPositionDirection());

    double constantTerm;
    if (ray->isPrimaryRayEnabled()) {
        if (!state.isViewpointCached()) {
            state.setViewpointValue(QuadricRayExpansion::constantTermWithoutMixed(
                surface, ray->getPosition2(), ray->getOrigin()));
            state.setViewpointCached(true);
        }
        constantTerm = state.getViewpointValue();
    } else {
        constantTerm = QuadricRayExpansion::constantTermWithoutMixed(
            surface, ray->getPosition2(), ray->getOrigin());
    }
    constantTerm = QuadricRayExpansion::constantTerm(surface, constantTerm,
        ray->getMixedPositionPosition());

    if (!QuadricSurface::solve(squareTerm, linearTerm, constantTerm, *depth1, *depth2) ||
        !clipToRenderDepths(depth1, depth2)) {
        return false;
    }

    stats.incrementRayQuadricTestsSucceeded();
    return true;
}

bool
QuadricTrace::clipToRenderDepths(double *depth1, double *depth2)
{
    if ((*depth1 < GeometryConfig::SMALL_TOLERANCE) || (*depth1 > GeometryConfig::MAX_DISTANCE)) {
        if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
            return false;
        }
        *depth1 = *depth2;
    } else if ((*depth2 < GeometryConfig::SMALL_TOLERANCE) || (*depth2 > GeometryConfig::MAX_DISTANCE)) {
        *depth2 = *depth1;
    }
    return true;
}

int
QuadricTrace::emitCrossings(QuadricSurface &surface, const SurfaceTraceState &state,
    RayWithTracingState *ray, java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    double depth1;
    double depth2;

    if (context == nullptr) {
        Vector3Dd intersectionPoint;
        bool intersectionFound = false;
        if (intersect(ray, surface, state, &depth1, &depth2)) {
            // Construct the candidate only on a real hit: ~89% of quadric tests
            // miss (drum cylinders), so deferring the ~168-byte zeroing past the
            // test removes it from the dominant miss path.
            IntersectionCandidate localElement;
            localElement.getIntersection().t = depth1;
            intersectionPoint = ray->getDirection().multiply(depth1);
            intersectionPoint = intersectionPoint.add(ray->getOrigin());
            localElement.getIntersection().point = intersectionPoint;
            localElement.getAttributes().setHitSurface(&surface, SurfaceDetailFamily::Quadric);
            localElement.getAttributes().setMaterial(materialOverride);
            depthQueue->offer(localElement);
            intersectionFound = true;

            if (depth2 != depth1) {
                localElement.getIntersection().t = depth2;
                intersectionPoint = ray->getDirection().multiply(depth2);
                intersectionPoint = intersectionPoint.add(ray->getOrigin());
                localElement.getIntersection().point = intersectionPoint;
                localElement.getAttributes().setHitSurface(&surface, SurfaceDetailFamily::Quadric);
                localElement.getAttributes().setMaterial(materialOverride);
                depthQueue->offer(localElement);
                intersectionFound = true;
            }
        }
        return intersectionFound;
    }

    if (!intersect(ray, surface, state, &depth1, &depth2)) {
        return false;
    }

    IntersectionCandidate localElement;
    localElement.getAttributes().setHitSurface(&surface, SurfaceDetailFamily::Quadric);
    localElement.getAttributes().setMaterial(context->getMaterialOverride());
    localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
    localElement.getAttributes().setMaterialUsesObjectLocalPoint(
        context->getMaterialUsesObjectLocalPoint());

    localElement.getIntersection().t = depth1;
    localElement.getIntersection().point =
        ray->getDirection().multiply(depth1).add(ray->getOrigin());
    depthQueue->offer(localElement);

    if (depth2 != depth1) {
        localElement.getIntersection().t = depth2;
        localElement.getIntersection().point =
            ray->getDirection().multiply(depth2).add(ray->getOrigin());
        depthQueue->offer(localElement);
    }

    return true;
}

bool
QuadricTrace::firstHit(QuadricSurface &surface, const SurfaceTraceState &state,
    RayWithTracingState *ray, IntersectionCandidate &out, Material *materialOverride)
{
    double depth1;
    double depth2;
    if (!intersect(ray, surface, state, &depth1, &depth2)) {
        return false;
    }

    const double nearestDepth = depth1 < depth2 ? depth1 : depth2;
    out.getIntersection().t = nearestDepth;
    out.getIntersection().point =
        ray->getDirection().multiply(nearestDepth).add(ray->getOrigin());
    out.getAttributes().setHitSurface(&surface, SurfaceDetailFamily::Quadric);
    out.getAttributes().setMaterial(materialOverride);
    return true;
}

int
QuadricTrace::containment(const QuadricSurface &surface, const Vector3Dd &point,
    double distanceTolerance)
{
    return surface.evaluate(point) < distanceTolerance ?
        static_cast<int>(Containment::INSIDE) : static_cast<int>(Containment::OUTSIDE);
}
