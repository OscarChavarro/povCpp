#ifndef VSDK_CSG_POINT_CLASSIFICATION_H
#define VSDK_CSG_POINT_CLASSIFICATION_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
class CsgExpression;
class Vector3Dd;
class CsgPointWitness;
class CsgPointProof;
class CsgPointAttribution;
class CsgOperandView;
class CsgPointWorkspace;

// Conservative world-space classification. An operand boundary becomes a
// composite boundary only when every other operand is certified non-interfering.
class CsgPointClassification {
private:
    static GeometryContainment classifyExpression(const CsgExpression& expression,
        const Vector3Dd& point, double tolerance, java::ArrayList<const CsgExpression*>& path,
        CsgPointWitness* witness, CsgPointProof* proof, CsgPointAttribution* attribution);
    static GeometryContainment containmentOfExpression(const CsgExpression& expression,
        const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace);
    static GeometryContainment containmentOfOperand(const CsgOperandView* operand,
        const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace);
    static void truncate(CsgPointWorkspace& workspace, long int resultCount);
public:
    static GeometryContainment classify(const CsgExpression& expression,
        const Vector3Dd& point, double tolerance);
    // Witness is cleared when no single operand proves the returned result.
    static GeometryContainment classifyWithWitness(const CsgExpression& expression,
        const Vector3Dd& point, double tolerance, CsgPointWitness& witness);
    // Collect sufficient leaf facts, including cases requiring several operands.
    static GeometryContainment classifyWithProof(const CsgExpression& expression,
        const Vector3Dd& point, double tolerance, CsgPointProof& proof);
    // All attributable owners are exposed only when the complete set is known.
    static GeometryContainment classifyWithAttribution(const CsgExpression& expression,
        const Vector3Dd& point, double tolerance, CsgPointAttribution& attribution);
    // The result of `classify` without collecting witnesses, proofs or
    // attributions, using the workspace's storage: it allocates nothing once
    // the workspace has grown to the expression. Equivalent operands share a
    // definite result as in `classify`.
    static GeometryContainment classifyContainment(const CsgExpression& expression,
        const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace);
    // `classifyContainment` of a single operand in its parent's space.
    static GeometryContainment classifyOperandContainment(const CsgOperandView& operand,
        const Vector3Dd& point, double tolerance, CsgPointWorkspace& workspace);
};
#endif
