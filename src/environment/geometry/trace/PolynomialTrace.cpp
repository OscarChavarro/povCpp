#include "java/lang/Math.h"
#include "environment/geometry/trace/PolynomialTrace.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "java/util/PriorityQueue.txx"

constexpr double PolynomialTrace::SHADOW_ROOT_MIN_DISTANCE;

int
PolynomialTrace::emitCrossings(PolynomialSurface &surface, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    double depths[PolynomialSurface::MAXIMUM_ORDER];
    Vector3Dd direction = ray->getDirection();
    const double len = java::Math::sqrt(direction.x() * direction.x() +
        direction.y() * direction.y() + direction.z() * direction.z());
    if (len == 0.0) {
        return 0;
    }
    direction = Vector3Dd(direction.x() / len, direction.y() / len, direction.z() / len);

    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayPolyTests();
    const int cnt = surface.intersect(ray->getOrigin(), direction,
        ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0, depths);
    if (cnt > 0) {
        stats.incrementRayPolyTestsSucceeded();
    }

    bool intersectionFound = false;
    IntersectionCandidate localElement;
    for (int i = 0; i < cnt; i++) {
        if (depths[i] < 0) {
            continue;
        }
        bool repeated = false;
        for (int j = 0; j < i; j++) {
            if (depths[i] == depths[j]) {
                repeated = true;
                break;
            }
        }
        if (repeated) {
            continue;
        }
        const Vector3Dd intersectionPoint =
            direction.multiply(depths[i]).add(ray->getOrigin());
        localElement.getIntersection().t =
            intersectionPoint.subtract(ray->getOrigin()).length();
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitSurface(&surface, SurfaceDetailFamily::Polynomial);
        if (context == nullptr) {
            localElement.getAttributes().setMaterial(materialOverride);
        } else {
            localElement.getAttributes().setMaterial(context->getMaterialOverride());
            localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
            localElement.getAttributes().setMaterialUsesObjectLocalPoint(
                context->getMaterialUsesObjectLocalPoint());
        }
        depthQueue->offer(localElement);
        intersectionFound = true;
    }
    return intersectionFound;
}

int
PolynomialTrace::containment(const PolynomialSurface &surface, const SurfaceTraceState &state,
    const Vector3Dd &point, double distanceTolerance)
{
    const int inside = static_cast<int>(Containment::INSIDE);
    const int outside = static_cast<int>(Containment::OUTSIDE);
    double result = surface.evaluate(point);
    // Points really near the surface are considered inside it.
    if (result > -GeometryConfig::INTERSECTION_EPSILON &&
        result < GeometryConfig::INTERSECTION_EPSILON) {
        result = 0.0;
    }
    if (result < distanceTolerance) {
        return state.isInverted() ? outside : inside;
    }
    return state.isInverted() ? inside : outside;
}
