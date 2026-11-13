#include <typeinfo>
#include "vsdk/toolkit/environment/scene/csg/CsgPlacedExpression.h"
#include "vsdk/toolkit/environment/scene/csg/CsgExpressionEquivalence.h"
CsgPlacedExpression::CsgPlacedExpression(const CsgExpression& expression) : child(expression), modificationVersion(0) {}
AffinePlacementStatus CsgPlacedExpression::setAffinePlacement(const Matrix4x4d& forward, const Matrix4x4d& inverse)
{
    AffinePlacement candidate;
    const AffinePlacementStatus status=candidate.assign(forward,inverse);
    if (status==AffinePlacementStatus::Valid) { placement=candidate; ++modificationVersion; }
    return status;
}
const Geometry* CsgPlacedExpression::getGeometry() const { return nullptr; }
Matrix4x4d CsgPlacedExpression::getAffinePlacement() const { return placement.getForward(); }
AffinePlacementStatus CsgPlacedExpression::queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const
{ forward=placement.getForward(); inverse=placement.getInverse(); return placement.getStatus(); }
const CsgExpression* CsgPlacedExpression::getChildExpression() const { return &child; }

bool CsgPlacedExpression::isSameSolid(const CsgOperandView& other) const
{ return CsgExpressionEquivalence::same(*this,other); }

bool CsgPlacedExpression::queryModificationVersion(long long& version) const
{ version=modificationVersion; return true; }

bool CsgPlacedExpression::isStructuralTraversalSafe() const { return typeid(*this)==typeid(CsgPlacedExpression); }
