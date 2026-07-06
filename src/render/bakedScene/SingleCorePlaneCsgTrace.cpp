
#include "render/bakedScene/SingleCorePlaneCsgTrace.h"

#include <cstdlib>

#include "render/bakedScene/CsgContainmentTest.h"
#include "render/bakedScene/CsgOperandTrace.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

bool
SingleCorePlaneCsgTrace::canUseCompiledSingleCorePlanePlan(
    const CsgProgram *bakedCsg,
    long int coreIndex)
{
    return
        bakedCsg->getPlanKind() ==
            BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
        bakedCsg->getSpecializationValid() &&
        coreIndex >= 0 &&
        coreIndex < bakedCsg->getOperands().size() &&
        bakedCsg->getPlaneOperandIndices().size() + 1 ==
            bakedCsg->getOperands().size();
}

bool
SingleCorePlaneCsgTrace::candidateInsideCompiledSingleCorePlaneOperands(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex,
    long int coreIndex)
{
    if (coreIndex >= 0 && coreIndex != skipIndex) {
        if (CsgContainmentTest::containmentTestOperand(
                bakedCsg->getOperands()[coreIndex],
                bakedCsgs,
                point,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }

    for (long int p = bakedCsg->getPlaneOperandIndices().size() - 1;
         p >= 0; p--) {
        const int operandIndex = bakedCsg->getPlaneOperandIndices()[p];
        if (operandIndex == skipIndex) {
            continue;
        }
        if (CsgContainmentTest::containmentTestOperand(
                bakedCsg->getOperands()[operandIndex],
                bakedCsgs,
                point,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
SingleCorePlaneCsgTrace::candidateInsideCompiledNestedContainmentSequence(
    const CsgOperandRecord *parentOperand,
    const CsgProgram *nestedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex)
{
    for (long int s =
             parentOperand->getCompiledNestedContainmentOperandIndices().size() - 1;
         s >= 0; s--) {
        const int operandIndex =
            parentOperand->getCompiledNestedContainmentOperandIndices()[s];
        if (operandIndex == skipIndex) {
            continue;
        }
        if (CsgContainmentTest::containmentTestOperand(
                nestedCsg->getOperands()[operandIndex],
                bakedCsgs,
                point,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

// Inlined containment check for the compiled direct-quadric core + direct-plane descriptor.
// Skips the containmentTestOperand switch for each check.
bool
SingleCorePlaneCsgTrace::candidateInsideDirectDescriptorOperands(
    const CsgOperandRecord *parentOperand,
    const CsgProgram *nestedCsg,
    const Vector3Dd &point,
    long int skipIndex)
{
    // Check planes in reverse order (same order as the compiled containment sequence
    // which stores [coreIndex, plane1Index, ...] iterated from the end).
    for (long int p = parentOperand->getCompiledNestedPlaneOperandIndices().size() - 1;
         p >= 0; p--) {
        const int planeIdx = parentOperand->getCompiledNestedPlaneOperandIndices()[p];
        if (planeIdx == skipIndex) {
            continue;
        }
        if (BakedPlaneIntersector::planeContainmentTest(
                nestedCsg->getOperands()[planeIdx],
                point,
                GeometryConfig::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    // Check core quadric last (it is stored first in the containment sequence, so
    // would be checked last in a reversed iteration).
    const long int coreIndex = parentOperand->getCompiledNestedCoreOperandIndex();
    if (skipIndex != coreIndex) {
        Quadric *quadric = nestedCsg->getOperands()[coreIndex]->getQuadricGeometry();
        if (quadric == nullptr ||
            quadric->doContainmentTest(point, GeometryConfig::SMALL_TOLERANCE) ==
                Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
SingleCorePlaneCsgTrace::traceCompiledCoreOperandAllCrossings(
    const CsgOperandRecord *operand,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (operand->getGeometry() == nullptr) {
        return false;
    }
    if (operand->getBounded() && operand->getCullSafe() &&
        !AabbCullingSupport::rayIntersectsAabbForward(*ray, operand->getBakedBounds())) {
        return false;
    }

    Material *effectiveMaterial =
        operand->getMaterial() != nullptr ? operand->getMaterial() : materialOverride;

    switch (operand->getKind()) {
    case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
    {
        GeometryIntersectionEmissionContext context(
            effectiveMaterial, operand->getOperand(), true);
        return operand->getGeometry()->doIntersectionForAllRayCrossingsAnnotated(
            ray, depthQueue, context);
    }

    case BakedScene::CsgOperandKind::DirectPrimitive:
    {
        const int initialSize = depthQueue->size();
        const bool found = operand->getGeometry()->doIntersectionForAllRayCrossings(
            ray, depthQueue, effectiveMaterial);
        if (!found || depthQueue->size() == initialSize) {
            return false;
        }
        return CsgOperandTrace::annotateDirectCandidates(depthQueue, operand);
    }

    case BakedScene::CsgOperandKind::TransformedQuadric:
    {
        const Vector3Dd localOrigin =
            operand->getLocalToObject().transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand->getLocalToObject().transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (operand->getQuadricGeometry() == nullptr) {
            return false;
        }
        if (!BakedQuadricIntersector::intersectBakedQuadric(
                *operand->getQuadricGeometry(),
                ray,
                localOrigin,
                localDirection,
                false,
                scratch.getCache(),
                operand->getQuadricViewpointSlot(),
                &depth1,
                &depth2)) {
            return false;
        }

        CsgOperandTrace::offerTransformedPrimitiveCandidate(
            operand,
            ray,
            effectiveMaterial,
            localOrigin,
            localDirection,
            depth1,
            depthQueue);
        if (depth2 != depth1) {
            CsgOperandTrace::offerTransformedPrimitiveCandidate(
                operand,
                ray,
                effectiveMaterial,
                localOrigin,
                localDirection,
                depth2,
                depthQueue);
        }
        return true;
    }

    case BakedScene::CsgOperandKind::TransformedSphere:
    {
        const Vector3Dd localOrigin =
            operand->getLocalToObject().transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand->getLocalToObject().transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (!Sphere::intersectSphereLocalSpace(
                localOrigin, localDirection, ray->getGeometryStatistics(),
                static_cast<Sphere *>(operand->getGeometry())->getRadius(),
                &depth1, &depth2)) {
            return false;
        }

        CsgOperandTrace::offerTransformedPrimitiveCandidate(
            operand, ray, effectiveMaterial,
            localOrigin, localDirection, depth1, depthQueue);
        if (depth2 != depth1) {
            CsgOperandTrace::offerTransformedPrimitiveCandidate(
                operand, ray, effectiveMaterial,
                localOrigin, localDirection, depth2, depthQueue);
        }
        return true;
    }

    default:
        return CsgOperandTrace::traceOperandAllCrossings(
            operand,
            bakedCsgs,
            scratch,
            ray,
            depthQueue,
            materialOverride);
    }
}

bool
SingleCorePlaneCsgTrace::offerTransformedQuadricCoreCandidate(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const CsgOperandRecord *coreOperand,
    RayWithTracingState *ray,
    Material *effectiveMaterial,
    const Vector3Dd &localOrigin,
    const Vector3Dd &localDirection,
    double depth,
    long int coreIndex,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    if (depth <= GeometryConfig::SMALL_TOLERANCE || depth >= GeometryConfig::MAX_DISTANCE) {
        return false;
    }

    IntersectionCandidate candidate;
    candidate.getIntersection().point =
        localOrigin.add(localDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(coreOperand->getGeometry());
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(coreOperand->getOperand());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        coreOperand->getObjectToLocal().transformPoint(candidate.getIntersection().point);
    const Vector3Dd rayOrigin = ray->getOrigin();
    const Vector3Dd rayDir = ray->getDirection();
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);

    if (!candidateInsideCompiledSingleCorePlaneOperands(
            bakedCsg,
            bakedCsgs,
            candidate.getIntersection().point,
            coreIndex,
            coreIndex)) {
        return false;
    }
    depthQueue->offer(candidate);
    return true;
}

bool
SingleCorePlaneCsgTrace::traceTransformedQuadricCorePlaneIntersection(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache,
    Material *materialOverride,
    long int coreIndex,
    bool &coreTrueMiss)
{
    coreTrueMiss = false;
    const CsgOperandRecord *coreOperand = bakedCsg->getOperands()[coreIndex];
    if (coreOperand->getKind() !=
        BakedScene::CsgOperandKind::TransformedQuadric) {
        return false;
    }
    if (coreOperand->getGeometry() == nullptr) {
        return false;
    }
    if (coreOperand->getBounded() && coreOperand->getCullSafe() &&
        !AabbCullingSupport::rayIntersectsAabbForward(*ray, coreOperand->getBakedBounds())) {
        return false;
    }

    Material *effectiveMaterial =
        coreOperand->getMaterial() != nullptr ? coreOperand->getMaterial() : materialOverride;
    const Vector3Dd localOrigin =
        coreOperand->getLocalToObject().transformPoint(ray->getOrigin());
    const Vector3Dd localDirection =
        coreOperand->getLocalToObject().transformDirection(ray->getDirection());

    double depth1;
    double depth2;
    if (coreOperand->getQuadricGeometry() == nullptr) {
        return false;
    }
    if (!BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
            *coreOperand->getQuadricGeometry(),
            ray,
            localOrigin,
            localDirection,
            false,
            cache,
            coreOperand->getQuadricViewpointSlot(),
            &depth1,
            &depth2,
            coreTrueMiss)) {
        return false;
    }

    bool anyIntersectionFound = false;
    if (offerTransformedQuadricCoreCandidate(
            bakedCsg,
            bakedCsgs,
            coreOperand,
            ray,
            effectiveMaterial,
            localOrigin,
            localDirection,
            depth1,
            coreIndex,
            depthQueue)) {
        anyIntersectionFound = true;
    }
    if (depth2 != depth1 &&
        offerTransformedQuadricCoreCandidate(
            bakedCsg,
            bakedCsgs,
            coreOperand,
            ray,
            effectiveMaterial,
            localOrigin,
            localDirection,
            depth2,
            coreIndex,
            depthQueue)) {
        anyIntersectionFound = true;
    }
    return anyIntersectionFound;
}

void
SingleCorePlaneCsgTrace::emitNestedCandidateToParentOperand(
    const CsgOperandRecord *parentOperand,
    RayWithTracingState *parentRay,
    const Matrix4x4d &nestedToParent,
    IntersectionCandidate &candidate,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    candidate.getAttributes().pushDetailOwner(parentOperand->getOperand());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        nestedToParent.transformPoint(candidate.getIntersection().point);
    const Vector3Dd rayOrigin = parentRay->getOrigin();
    const Vector3Dd rayDir = parentRay->getDirection();
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);
    depthQueue->offer(candidate);
}

bool
SingleCorePlaneCsgTrace::makeTransformedQuadricCandidateInRaySpace(
    const CsgOperandRecord *operand,
    RayWithTracingState *statsRay,
    Material *effectiveMaterial,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= GeometryConfig::SMALL_TOLERANCE || depth >= GeometryConfig::MAX_DISTANCE) {
        return false;
    }
    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        rayOrigin.add(rayDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(operand->getGeometry());
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand->getOperand());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        operand->getObjectToLocal().transformPoint(candidate.getIntersection().point);
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(statsRay->getOrigin())
            .dotProduct(statsRay->getDirection()) /
        statsRay->getDirection().dotProduct(statsRay->getDirection());
    return true;
}

bool
SingleCorePlaneCsgTrace::makeDirectQuadricCandidateInRaySpace(
    const CsgOperandRecord *operand,
    Material *effectiveMaterial,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= GeometryConfig::SMALL_TOLERANCE || depth >= GeometryConfig::MAX_DISTANCE) {
        return false;
    }
    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        rayOrigin.add(rayDirection.multiply(depth));
    candidate.getIntersection().t = depth;
    candidate.getAttributes().setHitGeometry(operand->getGeometry());
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand->getOperand());
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    return true;
}

bool
SingleCorePlaneCsgTrace::traceTransformedNestedSingleCorePlaneOperandAllCrossings(
    const CsgOperandRecord *parentOperand,
    const CsgProgram *nestedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    RayWithTracingState *parentRay,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache,
    Material *materialOverride)
{
    const long int coreIndex = parentOperand->getCompiledNestedCoreOperandIndex();
    if (!canUseCompiledSingleCorePlanePlan(nestedCsg, coreIndex)) {
        return false;
    }
    const CsgOperandRecord *coreOperand = nestedCsg->getOperands()[coreIndex];
    Quadric *directCoreQuadric =
        coreOperand->getKind() ==
            BakedScene::CsgOperandKind::DirectAnnotatedPrimitive ?
        coreOperand->getQuadricGeometry() :
        nullptr;
    const bool transformedCoreQuadric =
        coreOperand->getKind() ==
            BakedScene::CsgOperandKind::TransformedQuadric;
    if (!transformedCoreQuadric && directCoreQuadric == nullptr) {
        return false;
    }

    const Vector3Dd *nestedRayOriginPtr;
    const Vector3Dd *nestedRayDirectionPtr;
    Vector3Dd nestedRayOriginStorage;
    Vector3Dd nestedRayDirectionStorage;
    if (parentOperand->getPushdownFolded()) {
        nestedRayOriginPtr = &parentRay->getOrigin();
        nestedRayDirectionPtr = &parentRay->getDirection();
#ifdef PLAN17_PHASE1_ASSERT_MODE
        const Vector3Dd checkOrigin =
            parentOperand->getLocalToObject().transformPoint(parentRay->getOrigin());
        const Vector3Dd checkDirection =
            parentOperand->getLocalToObject().transformDirection(parentRay->getDirection());
        verifyBitwiseEqual(std::memcmp(&checkOrigin, nestedRayOriginPtr, sizeof(Vector3Dd)) == 0);
        verifyBitwiseEqual(std::memcmp(&checkDirection, nestedRayDirectionPtr, sizeof(Vector3Dd)) == 0);
#endif
    } else {
        nestedRayOriginStorage =
            parentOperand->getLocalToObject().transformPoint(parentRay->getOrigin());
        nestedRayDirectionStorage =
            parentOperand->getLocalToObject().transformDirection(parentRay->getDirection());
        nestedRayOriginPtr = &nestedRayOriginStorage;
        nestedRayDirectionPtr = &nestedRayDirectionStorage;
    }
    const Vector3Dd &nestedRayOrigin = *nestedRayOriginPtr;
    const Vector3Dd &nestedRayDirection = *nestedRayDirectionPtr;

    Material *effectiveCoreMaterial =
        coreOperand->getMaterial() != nullptr ? coreOperand->getMaterial() : materialOverride;
    const Vector3Dd coreRayOrigin = transformedCoreQuadric ?
        coreOperand->getLocalToObject().transformPoint(nestedRayOrigin) :
        nestedRayOrigin;
    const Vector3Dd coreRayDirection = transformedCoreQuadric ?
        coreOperand->getLocalToObject().transformDirection(nestedRayDirection) :
        nestedRayDirection;

    bool anyIntersectionFound = false;
    double depth1;
    double depth2;

    if (directCoreQuadric != nullptr) {
        bool trueMiss = false;
        const bool quadricHit = BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
            *directCoreQuadric,
            parentRay,
            coreRayOrigin,
            coreRayDirection,
            parentOperand->getPushdownFolded(),
            cache,
            coreOperand->getQuadricViewpointSlot(),
            &depth1,
            &depth2,
            trueMiss);

        if (trueMiss) {
            return false;
        }

        if (quadricHit) {
            IntersectionCandidate candidate;
            const bool madeDepth1 = makeDirectQuadricCandidateInRaySpace(
                coreOperand, effectiveCoreMaterial, coreRayOrigin,
                coreRayDirection, depth1, candidate);
            if (madeDepth1 &&
                candidateInsideDirectDescriptorOperands(
                    parentOperand, nestedCsg,
                    candidate.getIntersection().point,
                    coreIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand->getObjectToLocal(),
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
            if (depth2 != depth1) {
                const bool madeDepth2 = makeDirectQuadricCandidateInRaySpace(
                    coreOperand, effectiveCoreMaterial, coreRayOrigin,
                    coreRayDirection, depth2, candidate);
                if (madeDepth2 &&
                    candidateInsideDirectDescriptorOperands(
                        parentOperand, nestedCsg,
                        candidate.getIntersection().point,
                        coreIndex)) {
                    emitNestedCandidateToParentOperand(
                        parentOperand, parentRay, parentOperand->getObjectToLocal(),
                        candidate, depthQueue);
                    anyIntersectionFound = true;
                }
            }
        }

        for (long int p = parentOperand->getCompiledNestedPlaneOperandIndices().size() - 1;
             p >= 0; p--) {
            const int operandIndex = parentOperand->getCompiledNestedPlaneOperandIndices()[p];
            const CsgOperandRecord *operand = nestedCsg->getOperands()[operandIndex];
            IntersectionCandidate candidate;
            if (BakedPlaneIntersector::tracePlaneOperandCandidateInRaySpace(
                    operand, parentRay,
                    nestedRayOrigin, nestedRayDirection,
                    cache, materialOverride, candidate) &&
                candidateInsideDirectDescriptorOperands(
                    parentOperand, nestedCsg,
                    candidate.getIntersection().point,
                    operandIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand->getObjectToLocal(),
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
        }
    } else {
        // Transformed-quadric path: keep original code path.
        if (BakedQuadricIntersector::intersectBakedQuadric(
                *coreOperand->getQuadricGeometry(),
                parentRay,
                coreRayOrigin,
                coreRayDirection,
                false,
                cache,
                coreOperand->getQuadricViewpointSlot(),
                &depth1,
                &depth2)) {
            IntersectionCandidate candidate;
            const bool madeDepth1 = makeTransformedQuadricCandidateInRaySpace(
                coreOperand, parentRay, effectiveCoreMaterial, coreRayOrigin,
                coreRayDirection, depth1, candidate);
            if (madeDepth1 &&
                candidateInsideCompiledNestedContainmentSequence(
                    parentOperand, nestedCsg, bakedCsgs,
                    candidate.getIntersection().point, coreIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand->getObjectToLocal(),
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
            if (depth2 != depth1) {
                const bool madeDepth2 = makeTransformedQuadricCandidateInRaySpace(
                    coreOperand, parentRay, effectiveCoreMaterial, coreRayOrigin,
                    coreRayDirection, depth2, candidate);
                if (madeDepth2 &&
                    candidateInsideCompiledNestedContainmentSequence(
                        parentOperand, nestedCsg, bakedCsgs,
                        candidate.getIntersection().point, coreIndex)) {
                    emitNestedCandidateToParentOperand(
                        parentOperand, parentRay, parentOperand->getObjectToLocal(),
                        candidate, depthQueue);
                    anyIntersectionFound = true;
                }
            }
        }

        for (long int p = parentOperand->getCompiledNestedPlaneOperandIndices().size() - 1;
             p >= 0; p--) {
            const int operandIndex = parentOperand->getCompiledNestedPlaneOperandIndices()[p];
            const CsgOperandRecord *operand = nestedCsg->getOperands()[operandIndex];
            IntersectionCandidate candidate;
            if (BakedPlaneIntersector::tracePlaneOperandCandidateInRaySpace(
                    operand, parentRay,
                    nestedRayOrigin, nestedRayDirection,
                    cache, materialOverride, candidate) &&
                candidateInsideCompiledNestedContainmentSequence(
                    parentOperand, nestedCsg, bakedCsgs,
                    candidate.getIntersection().point, operandIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand->getObjectToLocal(),
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
        }
    }

    return anyIntersectionFound;
}

int
SingleCorePlaneCsgTrace::traceSingleCorePlaneIntersection(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    long int coreIndex)
{
    if (!bakedCsg->getSpecializationValid()) {
        return false;
    }

    bool anyIntersectionFound = false;
    bool coreTrueMiss = false;
    const CsgOperandRecord *coreOperand = bakedCsg->getOperands()[coreIndex];

    if (canUseCompiledSingleCorePlanePlan(bakedCsg, coreIndex)) {
        if (coreOperand->getKind() ==
            BakedScene::CsgOperandKind::TransformedQuadric) {
            anyIntersectionFound =
                traceTransformedQuadricCorePlaneIntersection(
                    bakedCsg,
                    bakedCsgs,
                    ray,
                    depthQueue,
                    scratch.getCache(),
                    materialOverride,
                    coreIndex,
                    coreTrueMiss);
        } else {
            java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
                scratch.borrowQueue();
            traceCompiledCoreOperandAllCrossings(
                coreOperand,
                bakedCsgs,
                scratch,
                ray,
                localDepthQueue,
                materialOverride);
            for (IntersectionCandidate &candidate : *localDepthQueue) {
                if (candidateInsideCompiledSingleCorePlaneOperands(
                        bakedCsg,
                        bakedCsgs,
                        candidate.getIntersection().point,
                        coreIndex,
                        coreIndex)) {
                    depthQueue->offer(candidate);
                    anyIntersectionFound = true;
                }
            }
            if (localDepthQueue->size() == 0 &&
                coreOperand->getQuadricGeometry() != nullptr) {
                double d1, d2;
                BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
                    *coreOperand->getQuadricGeometry(), ray,
                    ray->getOrigin(), ray->getDirection(),
                    true, scratch.getCache(), coreOperand->getQuadricViewpointSlot(),
                    &d1, &d2, coreTrueMiss);
            }
            scratch.returnQueue(localDepthQueue);
        }

        if (!coreTrueMiss) {
            for (long int p = bakedCsg->getPlaneOperandIndices().size() - 1;
                 p >= 0; p--) {
                const int operandIndex = bakedCsg->getPlaneOperandIndices()[p];
                const CsgOperandRecord *operand = bakedCsg->getOperands()[operandIndex];
                IntersectionCandidate candidate;
                if (BakedPlaneIntersector::tracePlaneOperandCandidate(
                        operand,
                        ray,
                        scratch.getCache(),
                        materialOverride,
                        candidate) &&
                    candidateInsideCompiledSingleCorePlaneOperands(
                        bakedCsg,
                        bakedCsgs,
                        candidate.getIntersection().point,
                        operandIndex,
                        coreIndex)) {
                    depthQueue->offer(candidate);
                    anyIntersectionFound = true;
                }
            }
        }

        return anyIntersectionFound;
    }

    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();

    for (long int i = bakedCsg->getOperands().size() - 1; i >= 0; i--) {
        const CsgOperandRecord *operand = bakedCsg->getOperands()[i];
        if (i == coreIndex) {
            localDepthQueue->clear();
            CsgOperandTrace::tracePlanOperandAllCrossings(
                coreOperand,
                bakedCsgs,
                scratch,
                ray,
                localDepthQueue,
                materialOverride);
            if (localDepthQueue->size() == 0) {
                continue;
            }

            for (IntersectionCandidate &candidate : *localDepthQueue) {
                if (CsgContainmentTest::candidateInsideOperandsCoreFirst(
                        bakedCsg,
                        bakedCsgs,
                        candidate.getIntersection().point,
                        coreIndex,
                        coreIndex)) {
                    depthQueue->offer(candidate);
                    anyIntersectionFound = true;
                }
            }
            continue;
        }

        if (operand->getIsInfinitePlane() && operand->getNestedCsgProgramIndex() < 0) {
            IntersectionCandidate candidate;
            if (BakedPlaneIntersector::tracePlaneOperandCandidate(
                    operand,
                    ray,
                    scratch.getCache(),
                    materialOverride,
                    candidate) &&
                CsgContainmentTest::candidateInsideOperandsCoreFirst(
                    bakedCsg,
                    bakedCsgs,
                    candidate.getIntersection().point,
                    i,
                    coreIndex)) {
                depthQueue->offer(candidate);
                anyIntersectionFound = true;
            }
            continue;
        }

        localDepthQueue->clear();
        CsgOperandTrace::tracePlanOperandAllCrossings(
            operand,
            bakedCsgs,
            scratch,
            ray,
            localDepthQueue,
            materialOverride);
        if (localDepthQueue->size() == 0) {
            continue;
        }

        for (IntersectionCandidate &candidate : *localDepthQueue) {
            if (CsgContainmentTest::candidateInsideAllOtherOperands(
                    bakedCsg,
                    bakedCsgs,
                    candidate.getIntersection().point,
                    i)) {
                depthQueue->offer(candidate);
                anyIntersectionFound = true;
            }
        }
    }

    scratch.returnQueue(localDepthQueue);
    return anyIntersectionFound;
}

bool
SingleCorePlaneCsgTrace::offerCompiledSingleCorePlaneFirstHitCandidate(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    const IntersectionCandidate &candidate,
    long int skipIndex,
    long int coreIndex,
    double &bestT,
    IntersectionCandidate &out)
{
    const double t = candidate.getIntersection().t;
    if (t <= GeometryConfig::SMALL_TOLERANCE || t >= bestT) {
        return false;
    }
    if (!candidateInsideCompiledSingleCorePlaneOperands(
            bakedCsg,
            bakedCsgs,
            candidate.getIntersection().point,
            skipIndex,
            coreIndex)) {
        return false;
    }
    out = candidate;
    bestT = t;
    return true;
}

bool
SingleCorePlaneCsgTrace::makeTransformedQuadricCandidate(
    const CsgOperandRecord *operand,
    RayWithTracingState *ray,
    Material *effectiveMaterial,
    const Vector3Dd &localOrigin,
    const Vector3Dd &localDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= GeometryConfig::SMALL_TOLERANCE || depth >= GeometryConfig::MAX_DISTANCE) {
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
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);
    return true;
}

bool
SingleCorePlaneCsgTrace::traceCompiledCoreFirstHitCandidates(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    Material *materialOverride,
    double &bestT,
    IntersectionCandidate &out,
    bool &coreTrueMiss)
{
    coreTrueMiss = false;
    const long int coreIndex = bakedCsg->getSpecializationCoreOperandIndex();
    const CsgOperandRecord *coreOperand = bakedCsg->getOperands()[coreIndex];
    Material *effectiveMaterial =
        coreOperand->getMaterial() != nullptr ? coreOperand->getMaterial() : materialOverride;

    if (coreOperand->getKind() ==
        BakedScene::CsgOperandKind::TransformedQuadric) {
        if (coreOperand->getQuadricGeometry() == nullptr) {
            return false;
        }
        if (coreOperand->getBounded() && coreOperand->getCullSafe() &&
            !AabbCullingSupport::rayIntersectsAabbForward(*ray, coreOperand->getBakedBounds())) {
            return false;
        }

        const Vector3Dd localOrigin =
            coreOperand->getLocalToObject().transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            coreOperand->getLocalToObject().transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (!BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
                *coreOperand->getQuadricGeometry(),
                ray,
                localOrigin,
                localDirection,
                false,
                scratch.getCache(),
                coreOperand->getQuadricViewpointSlot(),
                &depth1,
                &depth2,
                coreTrueMiss)) {
            return false;
        }

        bool found = false;
        IntersectionCandidate candidate;
        if (makeTransformedQuadricCandidate(
                coreOperand,
                ray,
                effectiveMaterial,
                localOrigin,
                localDirection,
                depth1,
                candidate) &&
            offerCompiledSingleCorePlaneFirstHitCandidate(
                bakedCsg,
                bakedCsgs,
                candidate,
                coreIndex,
                coreIndex,
                bestT,
                out)) {
            found = true;
        }
        if (depth2 != depth1 &&
            makeTransformedQuadricCandidate(
                coreOperand,
                ray,
                effectiveMaterial,
                localOrigin,
                localDirection,
                depth2,
                candidate) &&
            offerCompiledSingleCorePlaneFirstHitCandidate(
                bakedCsg,
                bakedCsgs,
                candidate,
                coreIndex,
                coreIndex,
                bestT,
                out)) {
            found = true;
        }
        return found;
    }

    java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
        scratch.borrowQueue();
    bool found = false;
    traceCompiledCoreOperandAllCrossings(
        coreOperand,
        bakedCsgs,
        scratch,
        ray,
        localDepthQueue,
        materialOverride);
    for (IntersectionCandidate &candidate : *localDepthQueue) {
        if (offerCompiledSingleCorePlaneFirstHitCandidate(
                bakedCsg,
                bakedCsgs,
                candidate,
                coreIndex,
                coreIndex,
                bestT,
                out)) {
            found = true;
        }
    }
    if (localDepthQueue->size() == 0 &&
        coreOperand->getQuadricGeometry() != nullptr) {
        double d1, d2;
        BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
            *coreOperand->getQuadricGeometry(), ray,
            ray->getOrigin(), ray->getDirection(),
            true, scratch.getCache(), coreOperand->getQuadricViewpointSlot(),
            &d1, &d2, coreTrueMiss);
    }
    scratch.returnQueue(localDepthQueue);
    return found;
}

bool
SingleCorePlaneCsgTrace::traceFirstHitCompiledSingleCorePlane(
    const CsgProgram *bakedCsg,
    const java::ArrayList<CsgProgram *> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    const long int coreIndex = bakedCsg->getSpecializationCoreOperandIndex();
    if (!canUseCompiledSingleCorePlanePlan(bakedCsg, coreIndex)) {
        return false;
    }

    double bestT = GeometryConfig::MAX_DISTANCE;
    bool coreTrueMiss = false;
    bool found = traceCompiledCoreFirstHitCandidates(
        bakedCsg,
        bakedCsgs,
        scratch,
        ray,
        materialOverride,
        bestT,
        out,
        coreTrueMiss);

    if (!coreTrueMiss) {
        for (long int p = bakedCsg->getPlaneOperandIndices().size() - 1;
             p >= 0; p--) {
            const int operandIndex = bakedCsg->getPlaneOperandIndices()[p];
            const CsgOperandRecord *operand = bakedCsg->getOperands()[operandIndex];
            IntersectionCandidate candidate;
            if (BakedPlaneIntersector::tracePlaneOperandCandidate(
                    operand,
                    ray,
                    scratch.getCache(),
                    materialOverride,
                    candidate) &&
                offerCompiledSingleCorePlaneFirstHitCandidate(
                    bakedCsg,
                    bakedCsgs,
                    candidate,
                    operandIndex,
                    coreIndex,
                    bestT,
                    out)) {
                found = true;
            }
        }
    }

    return found;
}

void
SingleCorePlaneCsgTrace::verifyBitwiseEqual(bool condition)
{
    if (!condition) {
        std::abort();
    }
}
