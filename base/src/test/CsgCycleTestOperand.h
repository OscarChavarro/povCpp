#ifndef VSDK_CSG_CYCLE_TEST_OPERAND_H
#define VSDK_CSG_CYCLE_TEST_OPERAND_H
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
class CsgCycleTestOperand : public CsgExpression, public CsgOperandView {
public:
    CsgOperation getOperation() const override;
    long int getOperandCount() const override;
    const CsgOperandView* getOperand(long int index) const override;
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
    const CsgExpression* getChildExpression() const override;
    bool isStructuralTraversalSafe() const override;
};
#endif
