#include "vsdk/toolkit/environment/scene/SimpleBody.h"
#include "vsdk/toolkit/environment/scene/csg/CsgBodyOperand.h"

CsgBodyOperand::CsgBodyOperand(const SimpleBody& value) : body(value) {}
const Geometry* CsgBodyOperand::getGeometry() const { return body.getGeometry(); }
Matrix4x4d CsgBodyOperand::getAffinePlacement() const { return body.getAffinePlacement(); }

AffinePlacementStatus CsgBodyOperand::queryAffinePlacement(Matrix4x4d& forward, Matrix4x4d& inverse) const
{ return body.queryAffinePlacement(forward, inverse); }
