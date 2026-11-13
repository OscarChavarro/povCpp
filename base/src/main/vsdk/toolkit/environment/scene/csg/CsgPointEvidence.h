#ifndef VSDK_CSG_POINT_EVIDENCE_H
#define VSDK_CSG_POINT_EVIDENCE_H
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
#include "vsdk/toolkit/environment/scene/csg/CsgPointWitness.h"
// A leaf fact and the coordinates/tolerance at which it was certified.
class CsgPointEvidence {
private:
    CsgPointWitness path;
    CsgPointWitness equivalentPath;
    bool equivalence;
    GeometryContainment classification;
    Vector3Dd point;
    double tolerance;
public:
    CsgPointEvidence();
    CsgPointEvidence(const CsgPointWitness& path, GeometryContainment classification,
        const Vector3Dd& point, double tolerance);
    // Records a proven identity between two sibling solid views; path is the
    // source of the certified classification, equivalentPath its recipient.
    CsgPointEvidence(const CsgPointWitness& source, const CsgPointWitness& equivalent,
        const Vector3Dd& point, double tolerance);
    bool isEquivalence() const;
    const CsgPointWitness& getEquivalentPath() const;
    bool prepend(const CsgOperandView* operand);
    void reverseOrientation();
    CsgPointWitness& getPath();
    const CsgPointWitness& getPath() const;
    GeometryContainment getClassification() const;
    const Vector3Dd& getPoint() const;
    double getTolerance() const;
};
#endif
