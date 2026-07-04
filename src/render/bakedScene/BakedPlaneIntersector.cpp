#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "render/bakedScene/BakedPlaneIntersector.h"

#include "common/Config.h"
#include "environment/geometry/Geometry.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"

bool
BakedPlaneIntersector::intersectBakedPlane(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *ray,
    const Vector3Dd &origin,
    const Vector3Dd &direction,
    RaySharedCache &cache,
    double *depth)
{
    Statistics &stats = *ray->getStatistics();
    stats.incrementRayPlaneTests();

    double normalDotOrigin;
    if (ray->isPrimaryRayEnabled()) {
        if (!cache.getPlaneViewpointConstant(operand.planeViewpointSlot, normalDotOrigin)) {
            normalDotOrigin = operand.planeNormal.dotProduct(origin);
            normalDotOrigin += operand.planeDistance;
            normalDotOrigin *= -1.0;
            cache.setPlaneViewpointConstant(operand.planeViewpointSlot, normalDotOrigin);
        }
    } else {
        normalDotOrigin = operand.planeNormal.dotProduct(origin);
        normalDotOrigin += operand.planeDistance;
        normalDotOrigin *= -1.0;
    }

    const double normalDotDirection =
        operand.planeNormal.dotProduct(direction);
    if (normalDotDirection < Config::SMALL_TOLERANCE &&
        normalDotDirection > -Config::SMALL_TOLERANCE) {
        return false;
    }

    *depth = normalDotOrigin / normalDotDirection;
    if (*depth >= Config::SMALL_TOLERANCE &&
        *depth <= Config::MAX_DISTANCE) {
        stats.incrementRayPlaneTestsSucceeded();
        return true;
    }
    return false;
}

int
BakedPlaneIntersector::planeContainmentTest(
    const BakedScene::CsgOperandRecord &operand,
    const Vector3Dd &point,
    double distanceTolerance)
{
    const double signedDistance =
        point.dotProduct(operand.planeNormal) + operand.planeDistance;
    return signedDistance <= distanceTolerance ? Geometry::INSIDE : Geometry::OUTSIDE;
}

bool
BakedPlaneIntersector::tracePlaneOperandCandidate(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *ray,
    RaySharedCache &cache,
    Material *materialOverride,
    IntersectionCandidate &candidate)
{
    if (!operand.isInfinitePlane || operand.nestedCsgProgramIndex >= 0 ||
        operand.geometry == nullptr) {
        return false;
    }

    Material *effectiveMaterial =
        operand.material != nullptr ? operand.material : materialOverride;
    if (operand.hasTransform) {
        const Vector3Dd localOrigin =
            operand.localToObject.transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand.localToObject.transformDirection(ray->getDirection());

        double depth;
        if (!intersectBakedPlane(
                operand,
                ray,
                localOrigin,
                localDirection,
                cache,
                &depth) ||
            depth <= Config::SMALL_TOLERANCE) {
            return false;
        }

        candidate = IntersectionCandidate();
        candidate.getIntersection().point =
            localOrigin.add(localDirection.multiply(depth));
        candidate.getAttributes().setHitGeometry(operand.geometry);
        candidate.getAttributes().setMaterial(effectiveMaterial);
        candidate.getAttributes().pushDetailOwner(operand.operand);
        candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
        candidate.getIntersection().point =
            operand.objectToLocal.transformPoint(candidate.getIntersection().point);
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
        depth <= Config::SMALL_TOLERANCE) {
        return false;
    }

    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        ray->getOrigin().add(ray->getDirection().multiply(depth));
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().t = depth;
    return true;
}

bool
BakedPlaneIntersector::tracePlaneOperandCandidateInRaySpace(
    const BakedScene::CsgOperandRecord &operand,
    RayWithSegments *statsRay,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    RaySharedCache &cache,
    Material *materialOverride,
    IntersectionCandidate &candidate)
{
    if (!operand.isInfinitePlane || operand.nestedCsgProgramIndex >= 0 ||
        operand.geometry == nullptr) {
        return false;
    }

    Material *effectiveMaterial =
        operand.material != nullptr ? operand.material : materialOverride;
    Vector3Dd localOrigin = rayOrigin;
    Vector3Dd localDirection = rayDirection;
    if (operand.hasTransform) {
        localOrigin = operand.localToObject.transformPoint(rayOrigin);
        localDirection = operand.localToObject.transformDirection(rayDirection);
    }

    double depth;
    if (!intersectBakedPlane(
            operand,
            statsRay,
            localOrigin,
            localDirection,
            cache,
            &depth) ||
        depth <= Config::SMALL_TOLERANCE) {
        return false;
    }

    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        localOrigin.add(localDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    if (operand.hasTransform) {
        candidate.getIntersection().point =
            operand.objectToLocal.transformPoint(candidate.getIntersection().point);
    }
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDirection) /
        rayDirection.dotProduct(rayDirection);
    return true;
}
