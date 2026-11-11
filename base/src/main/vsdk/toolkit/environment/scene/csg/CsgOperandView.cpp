#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"

CsgOperandView::~CsgOperandView() {}
AffinePlacementStatus CsgOperandView::queryAffinePlacement(Matrix4x4d&, Matrix4x4d&) const
{ return AffinePlacementStatus::Indeterminate; }
const CsgExpression* CsgOperandView::getChildExpression() const
{
    return dynamic_cast<const CsgExpression*>(getGeometry());
}
