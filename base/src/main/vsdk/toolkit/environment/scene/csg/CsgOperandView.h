#ifndef VSDK_CSG_OPERAND_VIEW_H
#define VSDK_CSG_OPERAND_VIEW_H

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/scene/AffinePlacementStatus.h"
class Geometry;
class CsgExpression;

// A borrowed operand view; its geometry, child expression and placement live
// at least as long as the expression that references this view.
class CsgOperandView {
public:
    virtual ~CsgOperandView();
    virtual const Geometry* getGeometry() const = 0;
    virtual Matrix4x4d getAffinePlacement() const = 0;
    // Output matrices are usable only when the returned status is Valid.
    virtual AffinePlacementStatus queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const;
    virtual const CsgExpression* getChildExpression() const;
};
#endif
