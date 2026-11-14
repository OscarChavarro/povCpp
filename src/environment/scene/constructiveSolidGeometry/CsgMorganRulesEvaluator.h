#ifndef __CSG_MORGAN_RULES_EVALUATOR__
#define __CSG_MORGAN_RULES_EVALUATOR__

#include "vsdk/toolkit/environment/scene/csg/CsgOperation.h"
#include "environment/scene/constructiveSolidGeometry/CsgOperand.h"

/**
The POV-Ray 1.0 CSG evaluator (`CsgBodyEvaluator::CROSSING_MEMBERSHIP`): a
union keeps every operand crossing, an intersection keeps the crossings of an
operand that lie inside all the others, and containment is the point
membership of the operands. A difference is parsed as an intersection with
inverted operands, and inversion follows De Morgan's rules.
*/
class CsgMorganRulesEvaluator {
  private:
    static bool insideOperand(const Vector3Dd &point, CsgOperand *operand);
    static int intersectionCrossings(
        RayWithTracingState *ray,
        const java::ArrayList<CsgOperand*> &operands,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);

  public:
    static int allRayCrossings(
        CsgOperation operation,
        const java::ArrayList<CsgOperand*> &operands,
        RayWithTracingState *ray,
        java::PriorityQueue<IntersectionCandidate> *depthQueue,
        Material *materialOverride);
    static bool contains(
        CsgOperation operation, const java::ArrayList<CsgOperand*> &operands, const Vector3Dd &point);
    // Inverts the operands and returns the complementary operation.
    static CsgOperation invert(CsgOperation operation, const java::ArrayList<CsgOperand*> &operands);
};

#endif
