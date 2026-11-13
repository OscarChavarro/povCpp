#ifndef VSDK_CSG_POINT_CLASSIFICATION_H
#define VSDK_CSG_POINT_CLASSIFICATION_H

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
class CsgExpression;
class Vector3Dd;
class CsgPointWitness;
class CsgPointProof;
class CsgPointAttribution;

// Conservative world-space classification. An operand boundary becomes a
// composite boundary only when every other operand is certified non-interfering.
class CsgPointClassification {
private:
    static GeometryContainment classifyExpression(const CsgExpression& expression,
        const Vector3Dd& point, double tolerance, java::ArrayList<const CsgExpression*>& path,
        CsgPointWitness* witness, CsgPointProof* proof, CsgPointAttribution* attribution);
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
};
#endif
