#ifndef __AFFINE_ELLIPSOID_MINIMUM_DISTANCE__
#define __AFFINE_ELLIPSOID_MINIMUM_DISTANCE__

#include "vsdk/toolkit/environment/geometry/element/GeometryDistanceStatus.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
class Matrix4x4d;
class Vector3Dd;

// Certified enclosure of the minimum world-space surface distance. A valid
// result is at most maxWidth wide; otherwise the status is Indeterminate and
// all outputs remain unchanged. The inverse is only a numerical seed hint.
class AffineEllipsoidMinimumDistance {
public:
    static GeometryDistanceStatus query(const Vector3Dd& point, double radius,
        const Matrix4x4d& forward, const Matrix4x4d& inverseHint, double maxWidth,
        GeometryContainment& side, double& lower, double& upper);
};

#endif
