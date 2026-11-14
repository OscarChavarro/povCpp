
#include "render/bakedScene/BakedPlaneIntersector.h"

#include "environment/geometry/element/GeometryConfig.h"
#include "vsdk/toolkit/environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

bool
BakedPlaneIntersector::intersectBakedPlane(
    const CsgOperandRecord *operand,
    RayWithTracingState *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    RaySharedCache &cache,
    double *depth)
{
    GeometryStatistics &stats = *ray->getGeometryStatistics();
    stats.incrementRayPlaneTests();

    double normalDotOrigin;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getPlaneViewpointConstant(operand->getPlaneViewpointSlot(), normalDotOrigin)) {
            normalDotOrigin = operand->getPlaneNormal().dotProduct(origin);
            normalDotOrigin += operand->getPlaneDistance();
            normalDotOrigin *= -1.0;
            cache.setPlaneViewpointConstant(operand->getPlaneViewpointSlot(), normalDotOrigin);
        }
    } else {
        normalDotOrigin = operand->getPlaneNormal().dotProduct(origin);
        normalDotOrigin += operand->getPlaneDistance();
        normalDotOrigin *= -1.0;
    }

    const double normalDotDirection =
        operand->getPlaneNormal().dotProduct(direction);
    if (InfinitePlane::intersectionDepth(normalDotOrigin, normalDotDirection,
            GeometryConfig::SMALL_TOLERANCE, GeometryConfig::SMALL_TOLERANCE,
            GeometryConfig::MAX_DISTANCE, false, *depth) == RayFirstHitStatus::Hit) {
        stats.incrementRayPlaneTestsSucceeded();
        return true;
    }
    return false;
}

int
BakedPlaneIntersector::planeContainmentTest(
    const CsgOperandRecord *operand,
    const Vector3Dd &point,
    double distanceTolerance)
{
    const double signedDistance =
        point.dotProduct(operand->getPlaneNormal()) + operand->getPlaneDistance();
    return signedDistance <= distanceTolerance ? RayCastingGeometry::INSIDE : RayCastingGeometry::OUTSIDE;
}

bool
BakedPlaneIntersector::tracePlaneOperandCandidate(
    const CsgOperandRecord *operand,
    RayWithTracingState *ray,
    RaySharedCache &cache,
    Material *materialOverride,
    IntersectionCandidate &candidate)
{
    if (!operand->getIsInfinitePlane() || operand->getNestedCsgProgramIndex() >= 0 ||
        operand->getGeometry() == nullptr) {
        return false;
    }

    Material *effectiveMaterial =
        operand->getMaterial() != nullptr ? operand->getMaterial() : materialOverride;
    if (operand->getHasTransform()) {
        const Vector3Dd localOrigin =
            operand->getLocalToObject().transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand->getLocalToObject().transformDirection(ray->getDirection());

        double depth;
        if (!intersectBakedPlane(
                operand,
                ray,
                localOrigin,
                localDirection,
                cache,
                &depth) ||
            depth <= GeometryConfig::SMALL_TOLERANCE) {
            return false;
        }

        candidate = IntersectionCandidate();
        candidate.getIntersection().point =
            localOrigin.add(localDirection.multiply(depth));
        candidate.getAttributes().setHitGeometry(operand->getGeometry());
        candidate.getAttributes().setMaterial(effectiveMaterial);
        candidate.getAttributes().pushDetailOwner(operand->getOperand());
        candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
        candidate.getIntersection().point =
            operand->getObjectToLocal().transformPoint(candidate.getIntersection().point);
        const Vector3Dd rayOrigin = ray->getOrigin();
        const Vector3Dd rayDir = ray->getDirection();
        candidate.getIntersection().t =
            candidate.getIntersection().point
                .subtract(rayOrigin).dotProduct(rayDir) /
            rayDir.dotProduct(rayDir);
        return true;
    }

    double depth;
    if (!intersectBakedPlane(
            operand,
            ray,
            ray->getOrigin(),
            ray->getDirection(),
            cache,
            &depth) ||
        depth <= GeometryConfig::SMALL_TOLERANCE) {
        return false;
    }

    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        ray->getOrigin().add(ray->getDirection().multiply(depth));
    candidate.getAttributes().setHitGeometry(operand->getGeometry());
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand->getOperand());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().t = depth;
    return true;
}

bool
BakedPlaneIntersector::tracePlaneOperandCandidateInRaySpace(
    const CsgOperandRecord *operand,
    RayWithTracingState *statsRay,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    RaySharedCache &cache,
    Material *materialOverride,
    IntersectionCandidate &candidate)
{
    if (!operand->getIsInfinitePlane() || operand->getNestedCsgProgramIndex() >= 0 ||
        operand->getGeometry() == nullptr) {
        return false;
    }

    Material *effectiveMaterial =
        operand->getMaterial() != nullptr ? operand->getMaterial() : materialOverride;
    Vector3Dd localOrigin = rayOrigin;
    Vector3Dd localDirection = rayDirection;
    if (operand->getHasTransform()) {
        localOrigin = operand->getLocalToObject().transformPoint(rayOrigin);
        localDirection = operand->getLocalToObject().transformDirection(rayDirection);
    }

    double depth;
    if (!intersectBakedPlane(
            operand,
            statsRay,
            localOrigin,
            localDirection,
            cache,
            &depth) ||
        depth <= GeometryConfig::SMALL_TOLERANCE) {
        return false;
    }

    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        localOrigin.add(localDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(operand->getGeometry());
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand->getOperand());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    if (operand->getHasTransform()) {
        candidate.getIntersection().point =
            operand->getObjectToLocal().transformPoint(candidate.getIntersection().point);
    }
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDirection) /
        rayDirection.dotProduct(rayDirection);
    return true;
}
