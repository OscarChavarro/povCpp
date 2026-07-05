#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

#include "render/bakedScene/SingleCorePlaneCsgTrace.h"

#include <cstdlib>
#include <cstring>

#include "common/Config.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "render/bakedScene/AabbCullingSupport.h"
#include "render/bakedScene/BakedPlaneIntersector.h"
#include "render/bakedScene/BakedQuadricIntersector.h"
#include "render/bakedScene/CsgContainmentTest.h"
#include "render/bakedScene/CsgOperandTrace.h"

bool
SingleCorePlaneCsgTrace::canUseCompiledSingleCorePlanePlan(
    const BakedScene::CsgProgram &bakedCsg,
    long int coreIndex)
{
    return
        bakedCsg.planKind ==
            BakedScene::CsgPlanKind::SingleCorePlaneIntersection &&
        bakedCsg.specializationValid &&
        coreIndex >= 0 &&
        coreIndex < bakedCsg.operands.size() &&
        bakedCsg.planeOperandIndices.size() + 1 ==
            bakedCsg.operands.size();
}

bool
SingleCorePlaneCsgTrace::candidateInsideCompiledSingleCorePlaneOperands(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex,
    long int coreIndex)
{
    if (coreIndex >= 0 && coreIndex != skipIndex) {
        if (CsgContainmentTest::containmentTestOperand(
                bakedCsg.operands[coreIndex],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }

    for (long int p = bakedCsg.planeOperandIndices.size() - 1;
         p >= 0; p--) {
        const int operandIndex = bakedCsg.planeOperandIndices[p];
        if (operandIndex == skipIndex) {
            continue;
        }
        if (CsgContainmentTest::containmentTestOperand(
                bakedCsg.operands[operandIndex],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
SingleCorePlaneCsgTrace::candidateInsideCompiledNestedContainmentSequence(
    const BakedScene::CsgOperandRecord &parentOperand,
    const BakedScene::CsgProgram &nestedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const Vector3Dd &point,
    long int skipIndex)
{
    for (long int s =
             parentOperand.compiledNestedContainmentOperandIndices.size() - 1;
         s >= 0; s--) {
        const int operandIndex =
            parentOperand.compiledNestedContainmentOperandIndices[s];
        if (operandIndex == skipIndex) {
            continue;
        }
        if (CsgContainmentTest::containmentTestOperand(
                nestedCsg.operands[operandIndex],
                bakedCsgs,
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

// Inlined containment check for the compiled direct-quadric core + direct-plane descriptor.
// Skips the containmentTestOperand switch for each check.
bool
SingleCorePlaneCsgTrace::candidateInsideDirectDescriptorOperands(
    const BakedScene::CsgOperandRecord &parentOperand,
    const BakedScene::CsgProgram &nestedCsg,
    const Vector3Dd &point,
    long int skipIndex)
{
    // Check planes in reverse order (same order as the compiled containment sequence
    // which stores [coreIndex, plane1Index, ...] iterated from the end).
    for (long int p = parentOperand.compiledNestedPlaneOperandIndices.size() - 1;
         p >= 0; p--) {
        const int planeIdx = parentOperand.compiledNestedPlaneOperandIndices[p];
        if (planeIdx == skipIndex) {
            continue;
        }
        if (BakedPlaneIntersector::planeContainmentTest(
                nestedCsg.operands[planeIdx],
                point,
                Config::SMALL_TOLERANCE) == Geometry::OUTSIDE) {
            return false;
        }
    }
    // Check core quadric last (it is stored first in the containment sequence, so
    // would be checked last in a reversed iteration).
    const long int coreIndex = parentOperand.compiledNestedCoreOperandIndex;
    if (skipIndex != coreIndex) {
        Quadric *quadric = nestedCsg.operands[coreIndex].quadricGeometry;
        if (quadric == nullptr ||
            quadric->doContainmentTest(point, Config::SMALL_TOLERANCE) ==
                Geometry::OUTSIDE) {
            return false;
        }
    }
    return true;
}

bool
SingleCorePlaneCsgTrace::traceCompiledCoreOperandAllCrossings(
    const BakedScene::CsgOperandRecord &operand,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride)
{
    if (operand.geometry == nullptr) {
        return false;
    }
    if (operand.bounded && operand.cullSafe &&
        !AabbCullingSupport::rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
        return false;
    }

    Material *effectiveMaterial =
        operand.material != nullptr ? operand.material : materialOverride;

    switch (operand.kind) {
    case BakedScene::CsgOperandKind::DirectAnnotatedPrimitive:
    {
        GeometryIntersectionEmissionContext context;
        context.materialOverride = effectiveMaterial;
        context.detailOwner = operand.operand;
        context.materialUsesObjectLocalPoint = true;
        return operand.geometry->doIntersectionForAllRayCrossingsAnnotated(
            ray, depthQueue, context);
    }

    case BakedScene::CsgOperandKind::DirectPrimitive:
    {
        const int initialSize = depthQueue->size();
        const bool found = operand.geometry->doIntersectionForAllRayCrossings(
            ray, depthQueue, effectiveMaterial);
        if (!found || depthQueue->size() == initialSize) {
            return false;
        }
        return CsgOperandTrace::annotateDirectCandidates(depthQueue, operand);
    }

    case BakedScene::CsgOperandKind::TransformedQuadric:
    {
        const Vector3Dd localOrigin =
            operand.localToObject.transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand.localToObject.transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (operand.quadricGeometry == nullptr) {
            return false;
        }
        if (!BakedQuadricIntersector::intersectBakedQuadric(
                *operand.quadricGeometry,
                ray,
                localOrigin,
                localDirection,
                false,
                scratch.getCache(),
                operand.quadricViewpointSlot,
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
            operand.localToObject.transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            operand.localToObject.transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (!Sphere::intersectSphereLocalSpace(
                localOrigin, localDirection, ray->getStatistics(),
                static_cast<Sphere *>(operand.geometry)->getRadius(),
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
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const BakedScene::CsgOperandRecord &coreOperand,
    RayWithTracingState *ray,
    Material *effectiveMaterial,
    const Vector3Dd &localOrigin,
    const Vector3Dd &localDirection,
    double depth,
    long int coreIndex,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    if (depth <= Config::SMALL_TOLERANCE || depth >= Config::MAX_DISTANCE) {
        return false;
    }

    IntersectionCandidate candidate;
    candidate.getIntersection().point =
        localOrigin.add(localDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(coreOperand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(coreOperand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        coreOperand.objectToLocal.transformPoint(candidate.getIntersection().point);
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
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache,
    Material *materialOverride,
    long int coreIndex,
    bool &coreTrueMiss)
{
    coreTrueMiss = false;
    const BakedScene::CsgOperandRecord &coreOperand = bakedCsg.operands[coreIndex];
    if (coreOperand.kind !=
        BakedScene::CsgOperandKind::TransformedQuadric) {
        return false;
    }
    if (coreOperand.geometry == nullptr) {
        return false;
    }
    if (coreOperand.bounded && coreOperand.cullSafe &&
        !AabbCullingSupport::rayIntersectsAabbForward(*ray, coreOperand.bakedBounds)) {
        return false;
    }

    Material *effectiveMaterial =
        coreOperand.material != nullptr ? coreOperand.material : materialOverride;
    const Vector3Dd localOrigin =
        coreOperand.localToObject.transformPoint(ray->getOrigin());
    const Vector3Dd localDirection =
        coreOperand.localToObject.transformDirection(ray->getDirection());

    double depth1;
    double depth2;
    if (coreOperand.quadricGeometry == nullptr) {
        return false;
    }
    if (!BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
            *coreOperand.quadricGeometry,
            ray,
            localOrigin,
            localDirection,
            false,
            cache,
            coreOperand.quadricViewpointSlot,
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
    const BakedScene::CsgOperandRecord &parentOperand,
    RayWithTracingState *parentRay,
    const Matrix4x4d &nestedToParent,
    IntersectionCandidate &candidate,
    java::PriorityQueue<IntersectionCandidate> *depthQueue)
{
    candidate.getAttributes().pushDetailOwner(parentOperand.operand);
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
    const BakedScene::CsgOperandRecord &operand,
    RayWithTracingState *statsRay,
    Material *effectiveMaterial,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= Config::SMALL_TOLERANCE || depth >= Config::MAX_DISTANCE) {
        return false;
    }
    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        rayOrigin.add(rayDirection.multiply(depth));
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    candidate.getIntersection().point =
        operand.objectToLocal.transformPoint(candidate.getIntersection().point);
    candidate.getIntersection().t =
        candidate.getIntersection().point.subtract(statsRay->getOrigin())
            .dotProduct(statsRay->getDirection()) /
        statsRay->getDirection().dotProduct(statsRay->getDirection());
    return true;
}

bool
SingleCorePlaneCsgTrace::makeDirectQuadricCandidateInRaySpace(
    const BakedScene::CsgOperandRecord &operand,
    Material *effectiveMaterial,
    const Vector3Dd &rayOrigin,
    const Vector3Dd &rayDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= Config::SMALL_TOLERANCE || depth >= Config::MAX_DISTANCE) {
        return false;
    }
    candidate = IntersectionCandidate();
    candidate.getIntersection().point =
        rayOrigin.add(rayDirection.multiply(depth));
    candidate.getIntersection().t = depth;
    candidate.getAttributes().setHitGeometry(operand.geometry);
    candidate.getAttributes().setMaterial(effectiveMaterial);
    candidate.getAttributes().pushDetailOwner(operand.operand);
    candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
    return true;
}

bool
SingleCorePlaneCsgTrace::traceTransformedNestedSingleCorePlaneOperandAllCrossings(
    const BakedScene::CsgOperandRecord &parentOperand,
    const BakedScene::CsgProgram &nestedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    RayWithTracingState *parentRay,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    RaySharedCache &cache,
    Material *materialOverride)
{
    const long int coreIndex = parentOperand.compiledNestedCoreOperandIndex;
    if (!canUseCompiledSingleCorePlanePlan(nestedCsg, coreIndex)) {
        return false;
    }
    const BakedScene::CsgOperandRecord &coreOperand = nestedCsg.operands[coreIndex];
    Quadric *directCoreQuadric =
        coreOperand.kind ==
            BakedScene::CsgOperandKind::DirectAnnotatedPrimitive ?
        coreOperand.quadricGeometry :
        nullptr;
    const bool transformedCoreQuadric =
        coreOperand.kind ==
            BakedScene::CsgOperandKind::TransformedQuadric;
    if (!transformedCoreQuadric && directCoreQuadric == nullptr) {
        return false;
    }

    const Vector3Dd *nestedRayOriginPtr;
    const Vector3Dd *nestedRayDirectionPtr;
    Vector3Dd nestedRayOriginStorage;
    Vector3Dd nestedRayDirectionStorage;
    if (parentOperand.pushdownFolded) {
        nestedRayOriginPtr = &parentRay->getOrigin();
        nestedRayDirectionPtr = &parentRay->getDirection();
#ifdef PLAN17_PHASE1_ASSERT_MODE
        const Vector3Dd checkOrigin =
            parentOperand.localToObject.transformPoint(parentRay->getOrigin());
        const Vector3Dd checkDirection =
            parentOperand.localToObject.transformDirection(parentRay->getDirection());
        verifyBitwiseEqual(std::memcmp(&checkOrigin, nestedRayOriginPtr, sizeof(Vector3Dd)) == 0);
        verifyBitwiseEqual(std::memcmp(&checkDirection, nestedRayDirectionPtr, sizeof(Vector3Dd)) == 0);
#endif
    } else {
        nestedRayOriginStorage =
            parentOperand.localToObject.transformPoint(parentRay->getOrigin());
        nestedRayDirectionStorage =
            parentOperand.localToObject.transformDirection(parentRay->getDirection());
        nestedRayOriginPtr = &nestedRayOriginStorage;
        nestedRayDirectionPtr = &nestedRayDirectionStorage;
    }
    const Vector3Dd &nestedRayOrigin = *nestedRayOriginPtr;
    const Vector3Dd &nestedRayDirection = *nestedRayDirectionPtr;

    Material *effectiveCoreMaterial =
        coreOperand.material != nullptr ? coreOperand.material : materialOverride;
    const Vector3Dd coreRayOrigin = transformedCoreQuadric ?
        coreOperand.localToObject.transformPoint(nestedRayOrigin) :
        nestedRayOrigin;
    const Vector3Dd coreRayDirection = transformedCoreQuadric ?
        coreOperand.localToObject.transformDirection(nestedRayDirection) :
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
            parentOperand.pushdownFolded,
            cache,
            coreOperand.quadricViewpointSlot,
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
                    parentOperand, parentRay, parentOperand.objectToLocal,
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
                        parentOperand, parentRay, parentOperand.objectToLocal,
                        candidate, depthQueue);
                    anyIntersectionFound = true;
                }
            }
        }

        for (long int p = parentOperand.compiledNestedPlaneOperandIndices.size() - 1;
             p >= 0; p--) {
            const int operandIndex = parentOperand.compiledNestedPlaneOperandIndices[p];
            const BakedScene::CsgOperandRecord &operand = nestedCsg.operands[operandIndex];
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
                    parentOperand, parentRay, parentOperand.objectToLocal,
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
        }
    } else {
        // Transformed-quadric path: keep original code path.
        if (BakedQuadricIntersector::intersectBakedQuadric(
                *coreOperand.quadricGeometry,
                parentRay,
                coreRayOrigin,
                coreRayDirection,
                false,
                cache,
                coreOperand.quadricViewpointSlot,
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
                    parentOperand, parentRay, parentOperand.objectToLocal,
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
                        parentOperand, parentRay, parentOperand.objectToLocal,
                        candidate, depthQueue);
                    anyIntersectionFound = true;
                }
            }
        }

        for (long int p = parentOperand.compiledNestedPlaneOperandIndices.size() - 1;
             p >= 0; p--) {
            const int operandIndex = parentOperand.compiledNestedPlaneOperandIndices[p];
            const BakedScene::CsgOperandRecord &operand = nestedCsg.operands[operandIndex];
            IntersectionCandidate candidate;
            if (BakedPlaneIntersector::tracePlaneOperandCandidateInRaySpace(
                    operand, parentRay,
                    nestedRayOrigin, nestedRayDirection,
                    cache, materialOverride, candidate) &&
                candidateInsideCompiledNestedContainmentSequence(
                    parentOperand, nestedCsg, bakedCsgs,
                    candidate.getIntersection().point, operandIndex)) {
                emitNestedCandidateToParentOperand(
                    parentOperand, parentRay, parentOperand.objectToLocal,
                    candidate, depthQueue);
                anyIntersectionFound = true;
            }
        }
    }

    return anyIntersectionFound;
}

int
SingleCorePlaneCsgTrace::traceSingleCorePlaneIntersection(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    java::PriorityQueue<IntersectionCandidate> *depthQueue,
    Material *materialOverride,
    long int coreIndex)
{
    if (!bakedCsg.specializationValid) {
        return false;
    }

    bool anyIntersectionFound = false;
    bool coreTrueMiss = false;
    const BakedScene::CsgOperandRecord &coreOperand = bakedCsg.operands[coreIndex];

    if (canUseCompiledSingleCorePlanePlan(bakedCsg, coreIndex)) {
        if (coreOperand.kind ==
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
                coreOperand.quadricGeometry != nullptr) {
                double d1, d2;
                BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
                    *coreOperand.quadricGeometry, ray,
                    ray->getOrigin(), ray->getDirection(),
                    true, scratch.getCache(), coreOperand.quadricViewpointSlot,
                    &d1, &d2, coreTrueMiss);
            }
            scratch.returnQueue(localDepthQueue);
        }

        if (!coreTrueMiss) {
            for (long int p = bakedCsg.planeOperandIndices.size() - 1;
                 p >= 0; p--) {
                const int operandIndex = bakedCsg.planeOperandIndices[p];
                const BakedScene::CsgOperandRecord &operand = bakedCsg.operands[operandIndex];
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

    for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
        const BakedScene::CsgOperandRecord &operand = bakedCsg.operands[i];
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

        if (operand.isInfinitePlane && operand.nestedCsgProgramIndex < 0) {
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
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    const IntersectionCandidate &candidate,
    long int skipIndex,
    long int coreIndex,
    double &bestT,
    IntersectionCandidate &out)
{
    const double t = candidate.getIntersection().t;
    if (t <= Config::SMALL_TOLERANCE || t >= bestT) {
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
    const BakedScene::CsgOperandRecord &operand,
    RayWithTracingState *ray,
    Material *effectiveMaterial,
    const Vector3Dd &localOrigin,
    const Vector3Dd &localDirection,
    double depth,
    IntersectionCandidate &candidate)
{
    if (depth <= Config::SMALL_TOLERANCE || depth >= Config::MAX_DISTANCE) {
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
        candidate.getIntersection().point.subtract(rayOrigin).dotProduct(rayDir) /
        rayDir.dotProduct(rayDir);
    return true;
}

bool
SingleCorePlaneCsgTrace::traceCompiledCoreFirstHitCandidates(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    Material *materialOverride,
    double &bestT,
    IntersectionCandidate &out,
    bool &coreTrueMiss)
{
    coreTrueMiss = false;
    const long int coreIndex = bakedCsg.specializationCoreOperandIndex;
    const BakedScene::CsgOperandRecord &coreOperand = bakedCsg.operands[coreIndex];
    Material *effectiveMaterial =
        coreOperand.material != nullptr ? coreOperand.material : materialOverride;

    if (coreOperand.kind ==
        BakedScene::CsgOperandKind::TransformedQuadric) {
        if (coreOperand.quadricGeometry == nullptr) {
            return false;
        }
        if (coreOperand.bounded && coreOperand.cullSafe &&
            !AabbCullingSupport::rayIntersectsAabbForward(*ray, coreOperand.bakedBounds)) {
            return false;
        }

        const Vector3Dd localOrigin =
            coreOperand.localToObject.transformPoint(ray->getOrigin());
        const Vector3Dd localDirection =
            coreOperand.localToObject.transformDirection(ray->getDirection());

        double depth1;
        double depth2;
        if (!BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
                *coreOperand.quadricGeometry,
                ray,
                localOrigin,
                localDirection,
                false,
                scratch.getCache(),
                coreOperand.quadricViewpointSlot,
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
        coreOperand.quadricGeometry != nullptr) {
        double d1, d2;
        BakedQuadricIntersector::intersectBakedQuadricWithTrueMiss(
            *coreOperand.quadricGeometry, ray,
            ray->getOrigin(), ray->getDirection(),
            true, scratch.getCache(), coreOperand.quadricViewpointSlot,
            &d1, &d2, coreTrueMiss);
    }
    scratch.returnQueue(localDepthQueue);
    return found;
}

bool
SingleCorePlaneCsgTrace::traceFirstHitCompiledSingleCorePlane(
    const BakedScene::CsgProgram &bakedCsg,
    const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
    CsgScratchContext &scratch,
    RayWithTracingState *ray,
    IntersectionCandidate &out,
    Material *materialOverride)
{
    const long int coreIndex = bakedCsg.specializationCoreOperandIndex;
    if (!canUseCompiledSingleCorePlanePlan(bakedCsg, coreIndex)) {
        return false;
    }

    double bestT = Config::MAX_DISTANCE;
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
        for (long int p = bakedCsg.planeOperandIndices.size() - 1;
             p >= 0; p--) {
            const int operandIndex = bakedCsg.planeOperandIndices[p];
            const BakedScene::CsgOperandRecord &operand = bakedCsg.operands[operandIndex];
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
