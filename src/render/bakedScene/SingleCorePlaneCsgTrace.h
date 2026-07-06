#ifndef __SINGLE_CORE_PLANE_CSG_TRACE__
#define __SINGLE_CORE_PLANE_CSG_TRACE__

#include "render/bakedScene/BakedScene.h"
#include "render/bakedScene/CsgScratchContext.h"

class SingleCorePlaneCsgTrace {
public:
    static bool canUseCompiledSingleCorePlanePlan(
        const CsgProgram *bakedCsg,
        long int coreIndex);

    static bool candidateInsideCompiledSingleCorePlaneOperands(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex,
        long int coreIndex);

    static bool candidateInsideCompiledNestedContainmentSequence(
        const CsgOperandRecord *parentOperand,
        const CsgProgram *nestedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        const Vector3Dd &point,
        long int skipIndex);

    static bool candidateInsideDirectDescriptorOperands(
        const CsgOperandRecord *parentOperand,
        const CsgProgram *nestedCsg,
        const Vector3Dd &point,
        long int skipIndex);

    static bool traceCompiledCoreOperandAllCrossings(
        const CsgOperandRecord *operand,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

    static bool offerTransformedQuadricCoreCandidate(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        const CsgOperandRecord *coreOperand,
        RayWithTracingState *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        long int coreIndex,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool traceTransformedQuadricCorePlaneIntersection(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache,
        Material *materialOverride,
        long int coreIndex,
        bool &coreTrueMiss);

    static void emitNestedCandidateToParentOperand(
        const CsgOperandRecord *parentOperand,
        RayWithTracingState *parentRay,
        const Matrix4x4d &nestedToParent,
        IntersectionCandidate &candidate,
        java::PriorityQueue<IntersectionCandidate> *depthQueue);

    static bool makeTransformedQuadricCandidateInRaySpace(
        const CsgOperandRecord *operand,
        RayWithTracingState *statsRay,
        Material *effectiveMaterial,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        double depth,
        IntersectionCandidate &candidate);

    static bool makeDirectQuadricCandidateInRaySpace(
        const CsgOperandRecord *operand,
        Material *effectiveMaterial,
        const Vector3Dd &rayOrigin,
        const Vector3Dd &rayDirection,
        double depth,
        IntersectionCandidate &candidate);

    static bool traceTransformedNestedSingleCorePlaneOperandAllCrossings(
        const CsgOperandRecord *parentOperand,
        const CsgProgram *nestedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        RayWithTracingState *parentRay,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        RaySharedCache &cache,
        Material *materialOverride);

    static int traceSingleCorePlaneIntersection(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride,
        long int coreIndex);

    static bool offerCompiledSingleCorePlaneFirstHitCandidate(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        const IntersectionCandidate &candidate,
        long int skipIndex,
        long int coreIndex,
        double &bestT,
        IntersectionCandidate &out);

    static bool makeTransformedQuadricCandidate(
        const CsgOperandRecord *operand,
        RayWithTracingState *ray,
        Material *effectiveMaterial,
        const Vector3Dd &localOrigin,
        const Vector3Dd &localDirection,
        double depth,
        IntersectionCandidate &candidate);

    static bool traceCompiledCoreFirstHitCandidates(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
        Material *materialOverride,
        double &bestT,
        IntersectionCandidate &out,
        bool &coreTrueMiss);

    static bool traceFirstHitCompiledSingleCorePlane(
        const CsgProgram *bakedCsg,
        const java::ArrayList<CsgProgram *> &bakedCsgs,
        CsgScratchContext &scratch,
        RayWithTracingState *ray,
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
