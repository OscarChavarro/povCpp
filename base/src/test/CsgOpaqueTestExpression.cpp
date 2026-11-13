#include "CsgOpaqueTestExpression.h"
CsgOpaqueTestExpression::CsgOpaqueTestExpression(const CsgOperandView& value) : operand(value) {}
CsgOperation CsgOpaqueTestExpression::getOperation() const { return CsgOperation::UNION; }
long int CsgOpaqueTestExpression::getOperandCount() const { return 1; }
const CsgOperandView* CsgOpaqueTestExpression::getOperand(long int index) const { return index==0 ? &operand : nullptr; }
