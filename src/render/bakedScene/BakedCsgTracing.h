#ifndef __BAKED_CSG_TRACING__
#define __BAKED_CSG_TRACING__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"
#include "environment/scene/Scene.h"
#include "environment/geometry/volume/constructiveSolidGeometry/RaySegments.h"
#include "common/Config.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "render/bakedScene/CsgScratchContext.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

class Quadric;

class BakedCsgTracing {
public:
    static bool traceAllCrossings(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride = nullptr);

    static bool traceFirstHit(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride = nullptr);

    static int containmentTest(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool intersectBakedQuadric(
        const Quadric &shape,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        double *depth1,
        double *depth2);

private:
    static bool traceAllCrossingsWithScratch(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool traceFirstHitWithScratch(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride);

    static int traceSingleCorePlaneIntersection(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        long int coreIndex);

    static bool canUseCompiledSingleCorePlanePlan(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        long int coreIndex);

    static bool traceTransformedNestedSingleCorePlaneOperandAllCrossings(
        const Scene::BakedCsgOperand &parentOperand,
        const Scene::BakedConstructiveSolidGeometry &nestedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        RayWithSegments *parentRay,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool rayIntersectsAabbForward(
        const RayWithSegments &ray,
        const AxisAlignedBox &box);

    static bool pointInsideAabb(
        const Vector3Dd &point,
        const AxisAlignedBox &box,
        double tolerance);

    static bool intersectBakedPlane(
        const Scene::BakedCsgOperand &operand,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        double *depth);

    static int planeContainmentTest(
        const Scene::BakedCsgOperand &operand,
        const Vector3Dd &point,
        double distanceTolerance);

    static void mixVectorTerms(
        Vector3Dd &a,
        const Vector3Dd &b,
        const Vector3Dd &c);

    static bool intersectBakedQuadricWithTrueMiss(
        const Quadric &shape,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        double *depth1,
        double *depth2,
        bool &trueMiss);

    static bool intersectBakedQuadricWithCoeffs(
        const Quadric &shape,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        double *depth1,
        double *depth2,
        double &polyA,
        double &polyB,
        double &polyC,
        bool &trueMiss);

    static void offerTransformedPrimitiveCandidate(
        const Scene::BakedCsgOperand &operand,
        RayWithSegments *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool annotateDirectCandidates(
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const Scene::BakedCsgOperand &operand);

    static bool traceOperandAllCrossings(
        const Scene::BakedCsgOperand &operand,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        if (operand.geometry == nullptr) {
            return false;
        }
        if (operand.bounded && operand.cullSafe &&
            !rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
            return false;
        }

        Material *effectiveMaterial =
            operand.material != nullptr ? operand.material : materialOverride;
        switch (operand.executionKind) {
        case Scene::BakedCsgOperandExecutionKind::DirectAnnotatedPrimitive:
        {
            GeometryIntersectionEmissionContext context;
            context.materialOverride = effectiveMaterial;
            context.detailOwner = operand.operand;
            context.materialUsesObjectLocalPoint = true;
            return operand.geometry->doIntersectionForAllRayCrossingsAnnotated(
                ray, depthQueue, context);
        }

        case Scene::BakedCsgOperandExecutionKind::DirectPrimitive: {
            const int initialSize = depthQueue->size();
            const bool found = operand.geometry->doIntersectionForAllRayCrossings(
                ray, depthQueue, effectiveMaterial);
            if (!found || depthQueue->size() == initialSize) {
                return false;
            }
            return annotateDirectCandidates(depthQueue, operand);
        }

        default:
            break;
        }

        RayWithSegments *localRayPtr = ray;
        if (operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedPlane ||
            operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedQuadric ||
            operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedSphere ||
            operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedPrimitive ||
            operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedNestedCsg) {
            if (operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedPlane) {
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
                        &depth) ||
                    depth <= Config::SMALL_TOLERANCE) {
                    return false;
                }

                IntersectionCandidate candidate;
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
                depthQueue->offer(candidate);
                return true;
            }

            if (operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedQuadric) {
                const Vector3Dd localOrigin =
                    operand.localToObject.transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand.localToObject.transformDirection(ray->getDirection());

                double depth1;
                double depth2;
                if (operand.quadricGeometry == nullptr) {
                    return false;
                }
                if (!intersectBakedQuadric(
                        *operand.quadricGeometry,
                        ray,
                        localOrigin,
                        localDirection,
                        &depth1,
                        &depth2)) {
                    return false;
                }

                offerTransformedPrimitiveCandidate(
                    operand,
                    ray,
                    effectiveMaterial,
                    localOrigin,
                    localDirection,
                    depth1,
                    depthQueue);
                if (depth2 != depth1) {
                    offerTransformedPrimitiveCandidate(
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

            if (operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedSphere) {
                const Vector3Dd localOrigin =
                    operand.localToObject.transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand.localToObject.transformDirection(ray->getDirection());

                double depth1;
                double depth2;
                if (!Sphere::intersectSphereLocalSpace(
                        localOrigin, localDirection,
                        ray->getStatistics(), &depth1, &depth2)) {
                    return false;
                }

                offerTransformedPrimitiveCandidate(
                    operand, ray, effectiveMaterial,
                    localOrigin, localDirection, depth1, depthQueue);
                if (depth2 != depth1) {
                    offerTransformedPrimitiveCandidate(
                        operand, ray, effectiveMaterial,
                        localOrigin, localDirection, depth2, depthQueue);
                }
                return true;
            }

            RayWithSegments localRay(RayWithSegments::LocalIntersectionClone{}, *ray);
            localRay.setOrigin(operand.localToObject.transformPoint(ray->getOrigin()));
            localRay.setDirection(operand.localToObject.transformDirection(ray->getDirection()));
            localRay.setQuadricConstantsCached(false);
            localRayPtr = &localRay;

            java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
                scratch.borrowQueue();

            bool found = false;
            if (operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::TransformedNestedCsg) {
                found = traceAllCrossingsWithScratch(
                    bakedCsgs[operand.bakedCsgIndex],
                    bakedCsgs,
                    scratch,
                    localRayPtr,
                    localDepthQueue,
                    effectiveMaterial);
            } else {
                found = operand.geometry->doIntersectionForAllRayCrossings(
                    localRayPtr, localDepthQueue, effectiveMaterial);
            }

            const Vector3Dd rayOrigin = ray->getOrigin();
            const Vector3Dd rayDir = ray->getDirection();
            const double dirLenSq = rayDir.dotProduct(rayDir);
            for (IntersectionCandidate &candidate : *localDepthQueue) {
                candidate.getAttributes().pushDetailOwner(operand.operand);
                candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
                candidate.getIntersection().point =
                    operand.objectToLocal.transformPoint(candidate.getIntersection().point);
                candidate.getIntersection().t =
                    candidate.getIntersection().point
                        .subtract(rayOrigin).dotProduct(rayDir) / dirLenSq;
                depthQueue->offer(candidate);
            }

            scratch.returnQueue(localDepthQueue);
            return found;
        }

        if (operand.executionKind == Scene::BakedCsgOperandExecutionKind::DirectPlane) {
            double depth;
            if (!intersectBakedPlane(
                    operand,
                    ray,
                    ray->getOrigin(),
                    ray->getDirection(),
                    &depth) ||
                depth <= Config::SMALL_TOLERANCE) {
                return false;
            }

            IntersectionCandidate candidate;
            candidate.getIntersection().point =
                ray->getOrigin().add(ray->getDirection().multiply(depth));
            candidate.getAttributes().setHitGeometry(operand.geometry);
            candidate.getAttributes().setMaterial(effectiveMaterial);
            candidate.getAttributes().pushDetailOwner(operand.operand);
            candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
            candidate.getIntersection().t = depth;
            depthQueue->offer(candidate);
            return true;
        }

        const Vector3Dd rayOrigin = ray->getOrigin();
        const Vector3Dd rayDir = ray->getDirection();
        const double dirLenSq = rayDir.dotProduct(rayDir);
        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            scratch.borrowQueue();
        bool found = false;
        if (operand.executionKind == Scene::BakedCsgOperandExecutionKind::NestedCsg) {
            found = traceAllCrossingsWithScratch(
                bakedCsgs[operand.bakedCsgIndex],
                bakedCsgs,
                scratch,
                ray,
                localDepthQueue,
                effectiveMaterial);
        } else {
            found = operand.geometry->doIntersectionForAllRayCrossings(
                ray, localDepthQueue, effectiveMaterial);
        }
        for (IntersectionCandidate &candidate : *localDepthQueue) {
            candidate.getAttributes().pushDetailOwner(operand.operand);
            candidate.getAttributes().setMaterialUsesObjectLocalPoint(true);
            candidate.getIntersection().t =
                candidate.getIntersection().point
                    .subtract(rayOrigin).dotProduct(rayDir) / dirLenSq;
            depthQueue->offer(candidate);
        }

        scratch.returnQueue(localDepthQueue);
        return found;
    }

    static int containmentTestOperand(
        const Scene::BakedCsgOperand &operand,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool tracePlanOperandAllCrossings(
        const Scene::BakedCsgOperand &operand,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static RaySegments buildRaySegments(
        const Scene::BakedCsgOperand &operand,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        Material *materialOverride);

    static bool combineUnion(bool insideLeft, bool insideRight);
    static bool combineIntersection(bool insideLeft, bool insideRight);
    static bool combineDifference(bool insideLeft, bool insideRight);

    static RaySegments mergeByMembership(
        const RaySegments &left,
        const RaySegments &right,
        bool (*combine)(bool insideLeft, bool insideRight));

    static int traceRaySegmentCsg(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool candidateInsideAllOtherOperands(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex);

    static bool candidateInsideOperandsCoreFirst(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex,
        long int coreIndex);

    static bool candidateInsideCompiledSingleCorePlaneOperands(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex,
        long int coreIndex);

    static bool candidateInsideCompiledNestedContainmentSequence(
        const Scene::BakedCsgOperand &parentOperand,
        const Scene::BakedConstructiveSolidGeometry &nestedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex);

    static bool candidateInsideDirectDescriptorOperands(
        const Scene::BakedCsgOperand &parentOperand,
        const Scene::BakedConstructiveSolidGeometry &nestedCsg,
        const Vector3Dd &point,
        long int skipIndex);

    static bool tracePlaneOperandCandidate(
        const Scene::BakedCsgOperand &operand,
        RayWithSegments *ray,
        Material *materialOverride,
        IntersectionCandidate &candidate);

    static bool tracePlaneOperandCandidateInRaySpace(
        const Scene::BakedCsgOperand &operand,
        RayWithSegments *statsRay,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        Material *materialOverride,
        IntersectionCandidate &candidate);

    static bool traceCompiledCoreOperandAllCrossings(
        const Scene::BakedCsgOperand &operand,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool offerTransformedQuadricCoreCandidate(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const Scene::BakedCsgOperand &coreOperand,
        RayWithSegments *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        long int coreIndex,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceTransformedQuadricCorePlaneIntersection(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        long int coreIndex,
        bool &coreTrueMiss);

    static void emitNestedCandidateToParentOperand(
        const Scene::BakedCsgOperand &parentOperand,
        RayWithSegments *parentRay,
        const Matrix4x4d &nestedToParent,
        IntersectionCandidate &candidate,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool makeTransformedQuadricCandidateInRaySpace(
        const Scene::BakedCsgOperand &operand,
        RayWithSegments *statsRay,
        Material *effectiveMaterial,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        double depth,
        IntersectionCandidate &candidate);

    static bool makeDirectQuadricCandidateInRaySpace(
        const Scene::BakedCsgOperand &operand,
        Material *effectiveMaterial,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        double depth,
        IntersectionCandidate &candidate);

    static int traceMorganIntersectionGeneric(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        // Pre-scan: if any positive-quadric (polyA>0) operand definitely misses the ray
        // (trueMiss), all points along the ray are outside that operand. No crossing from
        // any other operand can satisfy the containment check for it → the whole INTERSECTION
        // produces no valid candidates. Early exit saves 3 operand evals per X_Tube miss.
        for (long int i = 0; i < (long int)bakedCsg.operands.size(); i++) {
            const Scene::BakedCsgOperand &op = bakedCsg.operands[i];
            if (op.executionKind != Scene::BakedCsgOperandExecutionKind::TransformedQuadric ||
                op.quadricGeometry == nullptr) {
                continue;
            }
            const Vector3Dd localOrigin =
                op.localToObject.transformPoint(ray->getOrigin());
            const Vector3Dd localDirection =
                op.localToObject.transformDirection(ray->getDirection());
            double polyA = 0, polyB = 0, polyC = 0;
            bool trueMiss = false;
            double d1, d2;
            intersectBakedQuadricWithCoeffs(
                *op.quadricGeometry, ray, localOrigin, localDirection,
                &d1, &d2, polyA, polyB, polyC, trueMiss);
            if (trueMiss && polyA > 0.0) {
                return false;
            }
        }

        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            scratch.borrowQueue();
        bool anyIntersectionFound = false;

        for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
            const Scene::BakedCsgOperand &localShape = bakedCsg.operands[i];
            tracePlanOperandAllCrossings(
                localShape, bakedCsgs, scratch, ray, localDepthQueue, materialOverride);

            for (IntersectionCandidate &candidate : *localDepthQueue) {
                if (candidateInsideAllOtherOperands(
                        bakedCsg,
                        bakedCsgs,
                        candidate.getIntersection().point,
                        i)) {
                    depthQueue->offer(candidate);
                    anyIntersectionFound = true;
                }
            }
            localDepthQueue->clear();
        }

        scratch.returnQueue(localDepthQueue);
        return anyIntersectionFound;
    }

    static int traceMorganIntersection(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool traceGenericMorganUnion(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        bool anyFound = false;

        // Direct planes: inline emission, no dispatcher overhead.
        for (long int p = bakedCsg.executionPlanPlaneOperandIndices.size() - 1;
             p >= 0; p--) {
            const Scene::BakedCsgOperand &operand =
                bakedCsg.operands[bakedCsg.executionPlanPlaneOperandIndices[p]];
            IntersectionCandidate candidate;
            if (tracePlaneOperandCandidate(operand, ray, materialOverride, candidate)) {
                depthQueue->offer(candidate);
                anyFound = true;
            }
        }

        // Direct primitives: bounds check then direct geometry call.
        for (long int d = bakedCsg.executionPlanDirectPrimitiveOperandIndices.size() - 1;
             d >= 0; d--) {
            const Scene::BakedCsgOperand &operand =
                bakedCsg.operands[bakedCsg.executionPlanDirectPrimitiveOperandIndices[d]];
            if (operand.geometry == nullptr) {
                continue;
            }
            if (operand.bounded && operand.cullSafe &&
                !rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
                continue;
            }
            Material *effectiveMaterial =
                operand.material != nullptr ? operand.material : materialOverride;
            if (operand.executionKind ==
                Scene::BakedCsgOperandExecutionKind::DirectAnnotatedPrimitive) {
                GeometryIntersectionEmissionContext context;
                context.materialOverride = effectiveMaterial;
                context.detailOwner = operand.operand;
                context.materialUsesObjectLocalPoint = true;
                if (operand.geometry->doIntersectionForAllRayCrossingsAnnotated(
                        ray, depthQueue, context)) {
                    anyFound = true;
                }
            } else {
                const int initialSize = depthQueue->size();
                if (operand.geometry->doIntersectionForAllRayCrossings(
                        ray, depthQueue, effectiveMaterial) &&
                    depthQueue->size() > initialSize) {
                    if (annotateDirectCandidates(depthQueue, operand)) {
                        anyFound = true;
                    }
                }
            }
        }

        // Nested CSGs: use compiled dispatch (handles compiled vs. generic per ray kind).
        for (long int n = bakedCsg.executionPlanNestedOperandIndices.size() - 1;
             n >= 0; n--) {
            if (tracePlanOperandAllCrossings(
                    bakedCsg.operands[bakedCsg.executionPlanNestedOperandIndices[n]],
                    bakedCsgs,
                    scratch,
                    ray,
                    depthQueue,
                    materialOverride)) {
                anyFound = true;
            }
        }

        // Transformed primitives and quadrics: generic path (needs local ray).
        for (long int t = bakedCsg.executionPlanTransformedPrimitiveOperandIndices.size() - 1;
             t >= 0; t--) {
            if (traceOperandAllCrossings(
                    bakedCsg.operands[bakedCsg.executionPlanTransformedPrimitiveOperandIndices[t]],
                    bakedCsgs,
                    scratch,
                    ray,
                    depthQueue,
                    materialOverride)) {
                anyFound = true;
            }
        }

        return anyFound;
    }

    static int traceMorganCsg(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool offerCompiledSingleCorePlaneFirstHitCandidate(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        const IntersectionCandidate &candidate,
        long int skipIndex,
        long int coreIndex,
        double &bestT,
        IntersectionCandidate &out);

    static bool makeTransformedQuadricCandidate(
        const Scene::BakedCsgOperand &operand,
        RayWithSegments *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        IntersectionCandidate &candidate);

    static bool traceCompiledCoreFirstHitCandidates(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        Material *materialOverride,
        double &bestT,
        IntersectionCandidate &out,
        bool &coreTrueMiss);

    static bool traceFirstHitCompiledSingleCorePlane(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride);

    static bool traceFirstHitByIntersectionMembership(
        const Scene::BakedConstructiveSolidGeometry &bakedCsg,
        const java::ArrayList<Scene::BakedConstructiveSolidGeometry> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride);
};

#endif
