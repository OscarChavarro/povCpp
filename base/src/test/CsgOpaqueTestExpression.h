#ifndef VSDK_CSG_OPAQUE_TEST_EXPRESSION_H
#define VSDK_CSG_OPAQUE_TEST_EXPRESSION_H
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
class CsgOpaqueTestExpression : public CsgExpression {
private:
    const CsgOperandView& operand;
public:
    explicit CsgOpaqueTestExpression(const CsgOperandView& value);
    CsgOperation getOperation() const override;
    long int getOperandCount() const override;
    const CsgOperandView* getOperand(long int index) const override;
};
#endif
