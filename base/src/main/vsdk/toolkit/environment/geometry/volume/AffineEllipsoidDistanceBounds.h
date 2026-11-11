#ifndef __AFFINE_ELLIPSOID_DISTANCE_BOUNDS__
#define __AFFINE_ELLIPSOID_DISTANCE_BOUNDS__

#include "vsdk/toolkit/environment/geometry/element/GeometryDistanceStatus.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryContainment.h"
class Matrix4x4d;
class Vector3Dd;

// A certified enclosure for distance to the surface of an affine sphere.
// The lower bound uses a supporting plane or an interior inverse-norm bound;
// the upper bound uses a surface point in a nonzero local direction. Side may
// be Indeterminate. Outputs change only on Valid.
class AffineEllipsoidDistanceBounds {
public:
    static GeometryDistanceStatus query(const Vector3Dd& point, double radius,
        const Matrix4x4d& forward, const Vector3Dd& localDirection,
        GeometryContainment& side, double& lower, double& upper, bool refineInterior=true);
};

#endif
