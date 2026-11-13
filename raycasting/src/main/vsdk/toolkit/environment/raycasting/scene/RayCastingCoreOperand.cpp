#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingBody.h"
#include "vsdk/toolkit/environment/raycasting/scene/RayCastingCoreOperand.h"

RayCastingCoreOperand::RayCastingCoreOperand(const RayCastingBody& value) : body(value) {}

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
