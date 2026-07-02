#ifndef __BAKED_CSG_TRACING__
#define __BAKED_CSG_TRACING__

#include "environment/geometry/element/IntersectionCandidate.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "environment/material/Material.h"
#include "environment/scene/Scene.h"
#include "environment/geometry/volume/constructiveSolidGeometry/RaySegments.h"

class CsgScratchContext;
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

    static bool intersectBakedQuadric(
        const Quadric &shape,
        RayWithSegments *ray,
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        double *depth1,
        double *depth2);

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
        Material *materialOverride);

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
        long int coreIndex);

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
        Material *materialOverride);

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
        Material *materialOverride);

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
        IntersectionCandidate &out);

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
