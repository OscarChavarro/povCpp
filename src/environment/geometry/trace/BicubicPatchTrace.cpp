#include "environment/geometry/trace/BicubicPatchTrace.h"
#include "environment/geometry/element/GeometryConfig.h"
#include "environment/geometry/surface/parametric/PatchScratch.h"
#include "java/util/PriorityQueue.txx"

int
BicubicPatchTrace::emitCrossings(SubdividedBezierPatch &patch, RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride, const GeometryIntersectionEmissionContext *context)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayBicubicTests();
    PatchScratch &scratch = PatchScratch::forPatch(&patch);
    if (ray->isPrimaryRayEnabled()) {
        scratch.setIntersectionCount(0);
    }
    const int used = scratch.getIntersectionCount();
    double depths[MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd points[MAX_BICUBIC_INTERSECTIONS];
    Vector3Dd normals[MAX_BICUBIC_INTERSECTIONS];
    const int count = patch.intersect(ray->getOrigin(), ray->getDirection(),
        GeometryConfig::SMALL_TOLERANCE, used, MAX_BICUBIC_INTERSECTIONS, depths, points, normals);
    if (count <= 0) {
        return 0;
    }
    stats.incrementRayBicubicTestsSucceeded();
    IntersectionCandidate localElement;
    localElement.getAttributes().setHitSurface(&patch, SurfaceDetailFamily::BicubicPatch);
    localElement.getAttributes().setMaterial(materialOverride);
    if (context != nullptr) {
        localElement.getAttributes().pushDetailOwner(context->getDetailOwner());
        localElement.getAttributes().setMaterialUsesObjectLocalPoint(
            context->getMaterialUsesObjectLocalPoint());
    }
    for (int i = 0; i < count; i++) {
        scratch.getIntersectionPointAt(used + i) = points[i];
        scratch.getNormalVectorAt(used + i) = normals[i];
        if (!ray->isShadowRayEnabled()) {
            scratch.incrementIntersectionCount();
        }
        localElement.getIntersection().t = depths[i];
        localElement.getIntersection().point = points[i];
        localElement.getIntersection().normal = normals[i];
        depthQueue->offer(localElement);
    }
    return 1;
}
