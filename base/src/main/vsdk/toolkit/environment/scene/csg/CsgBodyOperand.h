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
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
};
#endif
