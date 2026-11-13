#ifndef VSDK_CSG_POLICY_TEST_OPERAND_H
#define VSDK_CSG_POLICY_TEST_OPERAND_H
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
class CsgPolicyTestOperand : public CsgOperandView {
private:
    const CsgExpression& expression;
    bool inverted;
public:
    CsgPolicyTestOperand(const CsgExpression& expression, bool inverted);
    bool isInverted() const;
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
    const CsgExpression* getChildExpression() const override;
};
#endif
