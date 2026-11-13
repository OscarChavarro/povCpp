#ifndef VSDK_CSG_BODY_OPERAND_H
#define VSDK_CSG_BODY_OPERAND_H

#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
class SimpleBody;

// Borrowed scene body; retains its existing geometry and affine placement.
class CsgBodyOperand : public CsgOperandView {
private:
    const SimpleBody& body;
public:
    explicit CsgBodyOperand(const SimpleBody& body);
    GeometryContainment classifyPointCertified(const Vector3Dd& point, double tolerance) const override;
    void getSolidBounds(GeometryBounds& result) const override;
    RaySolidQueryStatus classifyRay(const Ray& ray, const RayCrossingQuery& query,
        RaySolidClassification& result) const override;
    bool isSameSolid(const CsgOperandView& other) const override;
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
};
#endif
