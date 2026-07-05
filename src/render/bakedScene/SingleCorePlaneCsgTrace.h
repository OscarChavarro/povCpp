#ifndef __SINGLE_CORE_PLANE_CSG_TRACE__
#define __SINGLE_CORE_PLANE_CSG_TRACE__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"
#include "java/util/ArrayList.h"
#include "java/util/PriorityQueue.h"
#include "render/bakedScene/BakedScene.h"
#include "render/bakedScene/CsgScratchContext.h"
#include "render/raySharedCache/RaySharedCache.h"

class SingleCorePlaneCsgTrace {
public:
    static bool canUseCompiledSingleCorePlanePlan(
        const BakedScene::CsgProgram &bakedCsg,
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

    static bool traceTransformedNestedSingleCorePlaneOperandAllCrossings(
        const BakedScene::CsgOperandRecord &parentOperand,
        const BakedScene::CsgProgram &nestedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        RayWithSegments *parentRay,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache,
        Material *materialOverride);

    static int traceSingleCorePlaneIntersection(
        const BakedScene::CsgProgram &bakedCsg,
        const java::ArrayList<BakedScene::CsgProgram> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithSegments *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        long int coreIndex);

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

private:
    // Aborts if condition is false. Used only under
    // PLAN17_PHASE1_ASSERT_MODE to verify the pushdownFolded bit-equality
    // assumption exhaustively before shipping without it (see
    // traceTransformedNestedSingleCorePlaneOperandAllCrossings) - a plain
    // abort() instead of the standard assert() macro/<cassert>, since a
    // mismatch here means the phase's premise was wrong and must not be
    // silently tolerated (never "fixed" with a tolerance check).
    static void verifyBitwiseEqual(bool condition);
};

#endif
