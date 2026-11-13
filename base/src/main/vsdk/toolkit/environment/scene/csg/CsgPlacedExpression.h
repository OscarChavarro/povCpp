#ifndef VSDK_CSG_PLACED_EXPRESSION_H
#define VSDK_CSG_PLACED_EXPRESSION_H
#include "vsdk/toolkit/environment/scene/AffinePlacement.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
class CsgExpression;
// Borrowed child expression and an independently editable child-to-parent placement.
class CsgPlacedExpression : public CsgOperandView {
private:
    const CsgExpression& child;
    AffinePlacement placement;
public:
    explicit CsgPlacedExpression(const CsgExpression& child);
    AffinePlacementStatus setAffinePlacement(const Matrix4x4d& forward, const Matrix4x4d& inverse);
    bool isSameSolid(const CsgOperandView& other) const override;
    const Geometry* getGeometry() const override;
    Matrix4x4d getAffinePlacement() const override;
    AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const override;
    const CsgExpression* getChildExpression() const override;
    bool isStructuralTraversalSafe() const override;
};
#endif
