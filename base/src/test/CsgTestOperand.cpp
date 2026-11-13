#include "CsgTestOperand.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidClassification.h"
CsgTestOperand::CsgTestOperand(GeometryContainment value) : result(value), smallerResult(value), switchLimit(-1), malformedRay(false) {}
CsgTestOperand::CsgTestOperand(GeometryContainment value, GeometryContainment smaller, double limit)
    : result(value), smallerResult(smaller), switchLimit(limit), malformedRay(false) {}
const Geometry* CsgTestOperand::getGeometry() const { return nullptr; }
Matrix4x4d CsgTestOperand::getAffinePlacement() const { return Matrix4x4d::identityMatrix(); }
GeometryContainment CsgTestOperand::classifyPointCertified(const Vector3Dd&, double tolerance) const
{ return tolerance<=switchLimit ? smallerResult : result; }

void CsgTestOperand::setMalformedRay(bool value) { malformedRay=value; }
RaySolidQueryStatus CsgTestOperand::classifyRay(const Ray&, const RayCrossingQuery&, RaySolidClassification& result) const
{ result.clear(); return malformedRay ? RaySolidQueryStatus::Supported : RaySolidQueryStatus::Unsupported; }
