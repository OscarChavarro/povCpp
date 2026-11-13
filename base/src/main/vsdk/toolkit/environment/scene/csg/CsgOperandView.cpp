#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgOperandView.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"

CsgOperandView::~CsgOperandView() {}
AffinePlacementStatus CsgOperandView::queryAffinePlacement(Matrix4x4d&, Matrix4x4d&) const
{ return AffinePlacementStatus::Indeterminate; }
const CsgExpression* CsgOperandView::getChildExpression() const
{
    return dynamic_cast<const CsgExpression*>(getGeometry());
}

GeometryContainment CsgOperandView::classifyPointCertified(const Vector3Dd&, double) const
{ return GeometryContainment::Unsupported; }

bool CsgOperandView::isSameSolid(const CsgOperandView& other) const
{ return this==&other; }

RaySolidQueryStatus CsgOperandView::classifyRay(const Ray&, const RayCrossingQuery&, RaySolidClassification& result) const
{ result.clear(); return RaySolidQueryStatus::Unsupported; }

bool CsgOperandView::isStructuralTraversalSafe() const { return false; }

void CsgOperandView::getSolidBounds(GeometryBounds& result) const { result.setUnsupported(); }
