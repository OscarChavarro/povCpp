#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/geometry/volume/BlobComponent.h"

BlobComponent::BlobComponent() : BlobComponent(Vector3Dd(0, 0, 0), 1.0, 1.0)
{
}

BlobComponent::BlobComponent(const Vector3Dd& c, double r, double s) :
    center(c), radius(r), strength(s), radiusSquared(r*r), coefficients()
{
    coefficients[2] = s;
    coefficients[1] = -(2.0 * s) / radiusSquared;
    coefficients[0] = s / (radiusSquared * radiusSquared);
}

bool BlobComponent::isDegenerate() const
{
    return !java::Double::isFinite(radius) || !java::Double::isFinite(strength) ||
        !java::Double::isFinite(center.x()) || !java::Double::isFinite(center.y()) ||
        !java::Double::isFinite(center.z()) ||
        java::Math::abs(strength) < 1.0e-5 || radius < 1.0e-5;
}
