#ifndef VSDK_CSG_BODY_OPERAND_H
#define VSDK_CSG_BODY_OPERAND_H

#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
class SceneSolid;

// Borrowed scene body (any SceneSolid); retains its existing geometry and affine placement.
class CsgBodyOperand : public CsgOperandView {
private:
    const SceneSolid& body;
public:
    explicit CsgBodyOperand(const SceneSolid& body);
    GeometryContainment classifyPointCertified(const Vector3Dd& point, double tolerance) const override;
    void getSolidBounds(GeometryBounds& result) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    bool isSameSolid(const CsgOperandView& other) const override;
    // A body whose geometry is a CSG expression is traversed structurally only
    // when the body denotes exactly that placed geometry.
    bool isStructuralTraversalSafe() const override;
    bool queryModificationVersion(long long& version) const override;
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
};
#endif
