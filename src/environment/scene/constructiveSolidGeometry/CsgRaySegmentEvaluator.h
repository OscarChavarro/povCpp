#ifndef __CSG_RAY_SEGMENT_EVALUATOR__
#define __CSG_RAY_SEGMENT_EVALUATOR__

#include "vsdk/toolkit/environment/scene/csg/CsgOperation.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSegmentClassification.h"
#include "environment/scene/constructiveSolidGeometry/CsgOperand.h"

/**
The [ROTH1982] CSG evaluator (`CsgBodyEvaluator::ROTH`): Scott D. Roth, "Ray
Casting for Modeling Solids", CGIP 18, 109-144 (1982), section 3.3. Each
operand's ray crossings are classified in/out and merged per the Boolean
operator (Figs. 8-9, Table 3); DIFFERENCE is kept as its own operation.
*/
class CsgRaySegmentEvaluator {
  private:
    static CsgSegmentClassification<IntersectionCandidate> buildRaySegments(
        RayWithTracingState *ray, CsgOperand *operand, Material *materialOverride);
    // A top-level union of unbounded planes has no real in/out solid to
    // classify (Table 3 assumes bounded operands), so its crossings are not
    // merged.
    static bool isUnionOfBarePlanes(CsgOperation operation, const java::ArrayList<CsgOperand*> &operands);

  public:
    // The steps of RAYCAST shared with the baked render path. The point whose
    // containment classifies the ray before an operand's first crossing in
    // its sorted crossings (or just past the origin when it has none).
    static Vector3Dd classificationPoint(
        const RayWithTracingState &ray, java::PriorityQueue<IntersectionCandidate> &crossings);
    // Drains the sorted crossings into alternating in/out segments; segments
    // are merged by the shared CsgSegmentClassification::merge.
    static CsgSegmentClassification<IntersectionCandidate> segmentsFromCrossings(
        java::PriorityQueue<IntersectionCandidate> &crossings, bool initialInside);

    static int allRayCrossings(
        CsgOperation operation,
        const java::ArrayList<CsgOperand*> &operands,
        bool topLevel,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);
    static bool contains(
        CsgOperation operation,
        const java::ArrayList<CsgOperand*> &operands,
        const Vector3Dd &point,
        double distanceTolerance);
    // De Morgan complement of Table 3's three operators; returns the new
    // operation after inverting the operands it needs.
    static CsgOperation invert(CsgOperation operation, const java::ArrayList<CsgOperand*> &operands);
};

#endif
