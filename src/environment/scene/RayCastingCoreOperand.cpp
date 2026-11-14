#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "environment/scene/PovrayBody.h"
#include "environment/scene/RayCastingCoreOperand.h"

RayCastingCoreOperand::RayCastingCoreOperand(const PovrayBody& value) : body(value) {}

const Geometry* RayCastingCoreOperand::getGeometry() const
{
    const Geometry* intrinsic=nullptr; bool inverted=false;
    return body.queryCoreSolid(intrinsic,inverted) ? intrinsic : nullptr;
}

Matrix4x4d RayCastingCoreOperand::getAffinePlacement() const { return body.getAffinePlacement(); }

AffinePlacementStatus RayCastingCoreOperand::queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const
{ return body.queryAffinePlacement(forward,inverse); }

GeometryContainment RayCastingCoreOperand::classifyPointCertified(const Vector3Dd& point, double tolerance) const
{ return body.classifyCoreCertified(point,tolerance); }

void RayCastingCoreOperand::getSolidBounds(GeometryBounds& result) const { body.getCoreSolidWorldBounds(result); }

RaySolidQueryStatus RayCastingCoreOperand::classifyRay(const Ray& ray, const RayCrossingQuery& query,
    RaySolidClassification& result) const
{ return body.classifyCoreRay(ray,query,result); }

bool RayCastingCoreOperand::queryModificationVersion(long long& version) const
{ version=body.getModificationVersion(); return true; }
