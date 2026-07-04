#ifndef __BAKED_CSG_TRACE__
#define __BAKED_CSG_TRACE__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"
#include "environment/scene/Scene.h"
#include "environment/geometry/volume/constructiveSolidGeometry/RaySegments.h"
#include "common/Config.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/constructiveSolidGeometry/CsgOperand.h"
#include "environment/geometry/element/PriorityQueuePool.txx"
#include "render/bakedScene/BakedScene.h"
#include "render/bakedScene/CsgScratchContext.h"
#include "java/util/ArrayList.txx"
#include "java/util/PriorityQueue.txx"

class Quadric;

class BakedCsgTrace {
public:
    static bool traceAllCrossings(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache,
        Material *materialOverride = nullptr);

    static bool traceFirstHit(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        RayWithSegments *ray,
        IntersectionCandidate &out,
        RaySharedCache &cache,
        Material *materialOverride = nullptr);

    static int containmentTest(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool intersectBakedQuadric(
        const Quadric &shape,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        bool sharesRaySpace,
        RaySharedCache &cache,
        int viewpointSlot,
        double *depth1,
        double *depth2);

private:
    static bool traceAllCrossingsWithScratch(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool traceFirstHitWithScratch(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride);

    static int traceSingleCorePlaneIntersection(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        long int coreIndex);

    static bool canUseCompiledSingleCorePlanePlan(
        const BakedScene::CsgProgram &bakedCsg,
        long int coreIndex);

    static bool traceTransformedNestedSingleCorePlaneOperandAllCrossings(
        const BakedScene::CsgOperandRecord &parentOperand,
        const BakedScene::CsgProgram &nestedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        RayWithSegments *parentRay,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache,
        Material *materialOverride);

    // Defined inline here (not in BakedCsgTrace.cpp): this is the single
    // hottest leaf in the profile of AABB-culled scenes (Plan 11/12 gprof
    // sweep — 33-48% of self-time in union-heavy scenes like spline/ntreal,
    // called tens of millions of times per render) and LTO alone did not
    // fold it into its many call sites (still a distinct symbol post-LTO).
    // Header-inlining removes the out-of-line call itself, independent of
    // whatever the LTO pass decides.
    static inline bool rayIntersectsAabbForward(
        const RayWithSegments &ray,
        const AxisAlignedBox &box)
    {
        const Vector3Dd origin = ray.getOrigin();
        // Plan 12 Phase 3: the three reciprocals are cached on the ray
        // (invariant for as long as its origin/direction don't change), so
        // the slab test below no longer pays a division per axis per call.
        double invDirX, invDirY, invDirZ;
        bool degenerateX, degenerateY, degenerateZ;
        ray.getAabbSlabReciprocals(
            &invDirX, &invDirY, &invDirZ,
            &degenerateX, &degenerateY, &degenerateZ);
        double tMin = 0.0;
        double tMax = 1e30;

        auto updateAxis = [&](double originCoord, double invDir, bool degenerate,
                              double minCoord, double maxCoord) -> bool {
            if (degenerate) {
                return originCoord >= minCoord && originCoord <= maxCoord;
            }
            double nearT = (minCoord - originCoord) * invDir;
            double farT = (maxCoord - originCoord) * invDir;
            if (nearT > farT) {
                const double tmp = nearT;
                nearT = farT;
                farT = tmp;
            }
            tMin = nearT > tMin ? nearT : tMin;
            tMax = farT < tMax ? farT : tMax;
            return tMin <= tMax;
        };

        return
            updateAxis(origin.x(), invDirX, degenerateX, box.min.x(), box.max.x()) &&
            updateAxis(origin.y(), invDirY, degenerateY, box.min.y(), box.max.y()) &&
            updateAxis(origin.z(), invDirZ, degenerateZ, box.min.z(), box.max.z()) &&
            tMax >= 0.0;
    }

    static bool pointInsideAabb(
        const Vector3Dd &point,
        const AxisAlignedBox &box,
        double tolerance);

    static bool intersectBakedPlane(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        RaySharedCache &cache,
        double *depth);

    static int planeContainmentTest(
        const BakedScene::CsgOperandRecord &operand,
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
        bool sharesRaySpace,
        RaySharedCache &cache,
        int viewpointSlot,
        double *depth1,
        double *depth2,
        bool &trueMiss);

    static bool intersectBakedQuadricWithCoeffs(
        const Quadric &shape,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        bool sharesRaySpace,
        RaySharedCache &cache,
        int viewpointSlot,
        double *depth1,
        double *depth2,
        double &polyA,
        double &polyB,
        double &polyC,
        bool &trueMiss);

    static void offerTransformedPrimitiveCandidate(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool annotateDirectCandidates(
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        const BakedScene::CsgOperandRecord &operand);

    static bool traceOperandAllCrossings(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
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

        case BakedScene::CsgOperandKind::DirectPrimitive: {
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
        if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedPlane ||
            operand.kind ==
                BakedScene::CsgOperandKind::TransformedQuadric ||
            operand.kind ==
                BakedScene::CsgOperandKind::TransformedSphere ||
            operand.kind ==
                BakedScene::CsgOperandKind::TransformedPrimitive ||
            operand.kind ==
                BakedScene::CsgOperandKind::TransformedNestedCsg) {
            if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedPlane) {
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
                        scratch.getCache(),
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

            if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedQuadric) {
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
                        false,
                        scratch.getCache(),
                        operand.quadricViewpointSlot,
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

            if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedSphere) {
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
            if (operand.kind ==
                BakedScene::CsgOperandKind::TransformedNestedCsg) {
                found = traceAllCrossingsWithScratch(
                    bakedCsgs[operand.nestedCsgProgramIndex],
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

        if (operand.kind == BakedScene::CsgOperandKind::DirectPlane) {
            double depth;
            if (!intersectBakedPlane(
                    operand,
                    ray,
                    ray->getOrigin(),
                    ray->getDirection(),
                    scratch.getCache(),
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
        if (operand.kind == BakedScene::CsgOperandKind::NestedCsg) {
            found = traceAllCrossingsWithScratch(
                bakedCsgs[operand.nestedCsgProgramIndex],
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
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        double distanceTolerance);

    static bool tracePlanOperandAllCrossings(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static RaySegments buildRaySegments(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
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
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool candidateInsideAllOtherOperands(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex);

    static bool candidateInsideOperandsCoreFirst(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex,
        long int coreIndex);

    static bool candidateInsideCompiledSingleCorePlaneOperands(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex,
        long int coreIndex);

    static bool candidateInsideCompiledNestedContainmentSequence(
        const BakedScene::CsgOperandRecord &parentOperand,
        const BakedScene::CsgProgram &nestedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex);

    static bool candidateInsideDirectDescriptorOperands(
        const BakedScene::CsgOperandRecord &parentOperand,
        const BakedScene::CsgProgram &nestedCsg,
        const Vector3Dd &point,
        long int skipIndex);

    static bool tracePlaneOperandCandidate(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *ray,
        RaySharedCache &cache,
        Material *materialOverride,
        IntersectionCandidate &candidate);

    static bool tracePlaneOperandCandidateInRaySpace(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *statsRay,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        RaySharedCache &cache,
        Material *materialOverride,
        IntersectionCandidate &candidate);

    static bool traceCompiledCoreOperandAllCrossings(
        const BakedScene::CsgOperandRecord &operand,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool offerTransformedQuadricCoreCandidate(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const BakedScene::CsgOperandRecord &coreOperand,
        RayWithSegments *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        long int coreIndex,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceTransformedQuadricCorePlaneIntersection(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache,
        Material *materialOverride,
        long int coreIndex,
        bool &coreTrueMiss);

    static void emitNestedCandidateToParentOperand(
        const BakedScene::CsgOperandRecord &parentOperand,
        RayWithSegments *parentRay,
        const Matrix4x4d &nestedToParent,
        IntersectionCandidate &candidate,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool makeTransformedQuadricCandidateInRaySpace(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *statsRay,
        Material *effectiveMaterial,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        double depth,
        IntersectionCandidate &candidate);

    static bool makeDirectQuadricCandidateInRaySpace(
        const BakedScene::CsgOperandRecord &operand,
        Material *effectiveMaterial,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        double depth,
        IntersectionCandidate &candidate);

    // Plan 7 Phase 4: prescan and main loop both need each TransformedQuadric
    // operand's local-space intersection - without this cache the transform
    // (2 matrix-vector ops) and the quadric solve (~40 FLOPs) ran twice per
    // operand per ray. Cache scope is exactly this one function call (one CSG
    // program, one ray): a plain stack array indexed by the loop index `i`,
    // not the global RaySharedCache - no cross-call generation-stamp risk,
    // since the array cannot outlive the call that filled it. Capped at
    // MAX_CACHED_QUADRIC_OPERANDS; CSGs with more top-level operands than
    // that just skip the optimization and re-trace as before (always
    // correct, only forgoes the speedup).
    static constexpr long int MAX_CACHED_QUADRIC_OPERANDS = 64;

    static int traceMorganIntersectionGeneric(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        const long int operandCount = bakedCsg.operands.size();
        const bool canCache = operandCount <= MAX_CACHED_QUADRIC_OPERANDS;
        bool cachedValid[MAX_CACHED_QUADRIC_OPERANDS];
        bool cachedHit[MAX_CACHED_QUADRIC_OPERANDS];
        Vector3Dd cachedLocalOrigin[MAX_CACHED_QUADRIC_OPERANDS];
        Vector3Dd cachedLocalDirection[MAX_CACHED_QUADRIC_OPERANDS];
        double cachedDepth1[MAX_CACHED_QUADRIC_OPERANDS];
        double cachedDepth2[MAX_CACHED_QUADRIC_OPERANDS];
        if (canCache) {
            for (long int i = 0; i < operandCount; i++) {
                cachedValid[i] = false;
            }
        }

        // Pre-scan: if any positive-quadric (polyA>0) operand definitely misses the ray
        // (trueMiss), all points along the ray are outside that operand. No crossing from
        // any other operand can satisfy the containment check for it → the whole INTERSECTION
        // produces no valid candidates. Early exit saves 3 operand evals per X_Tube miss.
        for (long int i = 0; i < operandCount; i++) {
            const BakedScene::CsgOperandRecord &op = bakedCsg.operands[i];
            if (op.kind != BakedScene::CsgOperandKind::TransformedQuadric ||
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
            const bool hit = intersectBakedQuadricWithCoeffs(
                *op.quadricGeometry, ray, localOrigin, localDirection,
                false, scratch.getCache(), op.quadricViewpointSlot,
                &d1, &d2, polyA, polyB, polyC, trueMiss);
            if (trueMiss && polyA > 0.0) {
                return false;
            }
            if (canCache) {
                cachedValid[i] = true;
                cachedHit[i] = hit;
                cachedLocalOrigin[i] = localOrigin;
                cachedLocalDirection[i] = localDirection;
                cachedDepth1[i] = d1;
                cachedDepth2[i] = d2;
            }
        }

        java::PriorityQueue<IntersectionCandidate> *localDepthQueue =
            scratch.borrowQueue();
        bool anyIntersectionFound = false;

        for (long int i = bakedCsg.operands.size() - 1; i >= 0; i--) {
            const BakedScene::CsgOperandRecord &localShape = bakedCsg.operands[i];
            if (canCache && cachedValid[i]) {
                // Reuse the prescan's transform + quadric solve verbatim -
                // this mirrors traceOperandAllCrossings's TransformedQuadric
                // branch exactly (same effectiveMaterial rule, same
                // offerTransformedPrimitiveCandidate calls), just fed from
                // the cache instead of recomputing.
                if (cachedHit[i]) {
                    Material *effectiveMaterial =
                        localShape.material != nullptr ? localShape.material : materialOverride;
                    offerTransformedPrimitiveCandidate(
                        localShape, ray, effectiveMaterial,
                        cachedLocalOrigin[i], cachedLocalDirection[i],
                        cachedDepth1[i], localDepthQueue);
                    if (cachedDepth2[i] != cachedDepth1[i]) {
                        offerTransformedPrimitiveCandidate(
                            localShape, ray, effectiveMaterial,
                            cachedLocalOrigin[i], cachedLocalDirection[i],
                            cachedDepth2[i], localDepthQueue);
                    }
                }
            } else {
                tracePlanOperandAllCrossings(
                    localShape, bakedCsgs, scratch, ray, localDepthQueue, materialOverride);
            }

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
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool traceGenericMorganUnion(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride)
    {
        bool anyFound = false;

        // Direct planes: inline emission, no dispatcher overhead.
        for (long int p = bakedCsg.planeOperandIndices.size() - 1;
             p >= 0; p--) {
            const BakedScene::CsgOperandRecord &operand =
                bakedCsg.operands[bakedCsg.planeOperandIndices[p]];
            IntersectionCandidate candidate;
            if (tracePlaneOperandCandidate(
                    operand, ray, scratch.getCache(), materialOverride, candidate)) {
                depthQueue->offer(candidate);
                anyFound = true;
            }
        }

        // Direct primitives: bounds check then direct geometry call.
        for (long int d = bakedCsg.directPrimitiveOperandIndices.size() - 1;
             d >= 0; d--) {
            const BakedScene::CsgOperandRecord &operand =
                bakedCsg.operands[bakedCsg.directPrimitiveOperandIndices[d]];
            if (operand.geometry == nullptr) {
                continue;
            }
            if (operand.bounded && operand.cullSafe &&
                !rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
                continue;
            }
            Material *effectiveMaterial =
                operand.material != nullptr ? operand.material : materialOverride;
            if (operand.kind ==
                BakedScene::CsgOperandKind::DirectAnnotatedPrimitive) {
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
        for (long int n = bakedCsg.nestedOperandIndices.size() - 1;
             n >= 0; n--) {
            if (tracePlanOperandAllCrossings(
                    bakedCsg.operands[bakedCsg.nestedOperandIndices[n]],
                    bakedCsgs,
                    scratch,
                    ray,
                    depthQueue,
                    materialOverride)) {
                anyFound = true;
            }
        }

        // Transformed primitives and quadrics: TransformedQuadric (the
        // dominant residual-transformed kind) gets a direct fused branch
        // instead of paying traceOperandAllCrossings's function-call
        // boundary and generic kind dispatch (Plan 8 Phase 1); everything
        // else keeps the generic path. Iteration order is unchanged (same
        // array, same direction) so candidate/tie-break ordering across
        // operand kinds is preserved exactly.
        for (long int t = bakedCsg.transformedPrimitiveOperandIndices.size() - 1;
             t >= 0; t--) {
            const BakedScene::CsgOperandRecord &operand =
                bakedCsg.operands[bakedCsg.transformedPrimitiveOperandIndices[t]];
            if (operand.kind == BakedScene::CsgOperandKind::TransformedQuadric) {
                if (operand.geometry == nullptr || operand.quadricGeometry == nullptr) {
                    continue;
                }
                if (operand.bounded && operand.cullSafe &&
                    !rayIntersectsAabbForward(*ray, operand.bakedBounds)) {
                    continue;
                }
                Material *effectiveMaterial =
                    operand.material != nullptr ? operand.material : materialOverride;
                const Vector3Dd localOrigin =
                    operand.localToObject.transformPoint(ray->getOrigin());
                const Vector3Dd localDirection =
                    operand.localToObject.transformDirection(ray->getDirection());
                double depth1;
                double depth2;
                if (!intersectBakedQuadric(
                        *operand.quadricGeometry, ray, localOrigin, localDirection,
                        false, scratch.getCache(), operand.quadricViewpointSlot,
                        &depth1, &depth2)) {
                    continue;
                }
                offerTransformedPrimitiveCandidate(
                    operand, ray, effectiveMaterial,
                    localOrigin, localDirection, depth1, depthQueue);
                anyFound = true;
                if (depth2 != depth1) {
                    offerTransformedPrimitiveCandidate(
                        operand, ray, effectiveMaterial,
                        localOrigin, localDirection, depth2, depthQueue);
                }
                continue;
            }
            if (traceOperandAllCrossings(
                    operand,
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
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool offerCompiledSingleCorePlaneFirstHitCandidate(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        const IntersectionCandidate &candidate,
        long int skipIndex,
        long int coreIndex,
        double &bestT,
        IntersectionCandidate &out);

    static bool makeTransformedQuadricCandidate(
        const BakedScene::CsgOperandRecord &operand,
        RayWithSegments *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        IntersectionCandidate &candidate);

    static bool traceCompiledCoreFirstHitCandidates(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        Material *materialOverride,
        double &bestT,
        IntersectionCandidate &out,
        bool &coreTrueMiss);

    static bool traceFirstHitCompiledSingleCorePlane(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride);

    static bool traceFirstHitByIntersectionMembership(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        IntersectionCandidate &out,
        Material *materialOverride);
};

#endif
