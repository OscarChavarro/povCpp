#include "vsdk/toolkit/environment/scene/SceneSolid.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/scene/csg/CsgSolidEquivalence.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"

CsgBodyOperand::CsgBodyOperand(const SceneSolid& value) : body(value) {}
const Geometry* CsgBodyOperand::getGeometry() const { return body.getGeometry(); }
Matrix4x4d CsgBodyOperand::getAffinePlacement() const { return body.getAffinePlacement(); }

AffinePlacementStatus CsgBodyOperand::queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const
{ return body.queryAffinePlacement(forward, inverse); }

GeometryContainment CsgBodyOperand::classifyPointCertified(const Vector3Dd& point, double tolerance) const
{ return body.classifyPointCertified(point, tolerance); }

void CsgBodyOperand::getSolidBounds(GeometryBounds& result) const { body.getSolidWorldBounds(result); }

bool CsgBodyOperand::isSameSolid(const CsgOperandView& other) const
{
    const CsgBodyOperand* bodyOperand=dynamic_cast<const CsgBodyOperand*>(&other);
    return bodyOperand && CsgSolidEquivalence::same(body,bodyOperand->body);
}

RaySolidQueryStatus CsgBodyOperand::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{ return body.classifyRay(ray,query,result); }

bool CsgBodyOperand::isStructuralTraversalSafe() const { return body.isStructuralTraversalSafe(); }

bool CsgBodyOperand::queryModificationVersion(long long& version) const
{ version=body.getModificationVersion(); return true; }
