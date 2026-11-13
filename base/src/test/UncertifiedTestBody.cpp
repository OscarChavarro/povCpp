#include "UncertifiedTestBody.h"

GeometryContainment UncertifiedTestBody::classifyPointCertified(const Vector3Dd&, double) const
{ return GeometryContainment::Unsupported; }

const Geometry* UncertifiedTestBody::getSolidEquivalenceGeometry() const { return getGeometry(); }
