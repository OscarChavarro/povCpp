#include "java/lang/Math.h"
#include "java/util/ArrayList.txx"
#include "environment/geometry/trace/BlobTrace.h"
#include "vsdk/toolkit/processing/Containment.h"
#include "java/util/PriorityQueue.txx"

constexpr double BlobTrace::INSIDE_TOLERANCE;
constexpr double BlobTrace::SHADOW_ROOT_MIN_DISTANCE;
constexpr double BlobTrace::INFLUENCE_START;

int
BlobTrace::emitCrossings(BlobSolid &solid, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayBlobTests();
    const Vector3Dd p = ray->getOrigin();
    Vector3Dd d = ray->getDirection();
    const double len = java::Math::sqrt(d.x() * d.x() + d.y() * d.y() + d.z() * d.z());
    if (len == 0.0) {
        return 0;
    }
    d = Vector3Dd(d.x() / len, d.y() / len, d.z() / len);
    java::ArrayList<double> distances;
    solid.intersect(p, d, ray->isShadowRayEnabled() ? SHADOW_ROOT_MIN_DISTANCE : 0.0,
        INFLUENCE_START, distances);
    IntersectionCandidate localElement;
    for (long int j = 0; j < distances.size(); j++) {
        const Vector3Dd intersectionPoint = d.multiply(distances[j]).add(p);
        localElement.getIntersection().t = intersectionPoint.subtract(ray->getOrigin()).length();
        localElement.getIntersection().point = intersectionPoint;
        localElement.getAttributes().setHitSurface(&solid, SurfaceDetailFamily::Blob);
        localElement.getAttributes().setMaterial(
            context != nullptr ? context->getMaterialOverride() : materialOverride);
        if (context != nullptr) {
            localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
            localElement.getAttributes().setMaterialUsesObjectLocalPoint(
                context->getMaterialUsesObjectLocalPoint());
        }
        depthQueue->offer(localElement);
    }
    if (!distances.isEmpty()) {
        stats.incrementRayBlobTestsSucceeded();
    }
    return !distances.isEmpty();
}

int
BlobTrace::containment(const BlobSolid &solid, const SurfaceTraceState &state,
    const Vector3Dd &point, double distanceTolerance)
{
    (void)distanceTolerance;
    const int inside = static_cast<int>(Containment::INSIDE);
    const int outside = static_cast<int>(Containment::OUTSIDE);
    // INSIDE_TOLERANCE is a density margin of the blob field, not the
    // geometric point-to-surface tolerance used elsewhere.
    if (solid.evaluateDensity(point) > solid.getThreshold() - INSIDE_TOLERANCE) {
        return state.isInverted() ? outside : inside;
    }
    return state.isInverted() ? inside : outside;
}
