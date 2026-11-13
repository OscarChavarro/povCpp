#include "CsgCycleTestOperand.h"
CsgOperation CsgCycleTestOperand::getOperation() const { return CsgOperation::UNION; }
long int CsgCycleTestOperand::getOperandCount() const { return 1; }
const CsgOperandView* CsgCycleTestOperand::getOperand(long int index) const { return index==0 ? this : nullptr; }
const Geometry* CsgCycleTestOperand::getGeometry() const { return nullptr; }
Matrix4x4d CsgCycleTestOperand::getAffinePlacement() const { return Matrix4x4d::identityMatrix(); }
AffinePlacementStatus CsgCycleTestOperand::queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const
{ forward=Matrix4x4d::identityMatrix(); inverse=forward; return AffinePlacementStatus::Valid; }
const CsgExpression* CsgCycleTestOperand::getChildExpression() const { return this; }

bool CsgCycleTestOperand::isStructuralTraversalSafe() const { return true; }
