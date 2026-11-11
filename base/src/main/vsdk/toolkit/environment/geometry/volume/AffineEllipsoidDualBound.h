#ifndef __AFFINE_ELLIPSOID_DUAL_BOUND__
#define __AFFINE_ELLIPSOID_DUAL_BOUND__

class Matrix4x4d;
class Vector3Dd;

// Verified lower bound from a Lagrange dual certificate. The supplied local
// direction is only a numerical candidate; failure to certify returns zero.
class AffineEllipsoidDualBound {
public:
    static double lowerDistance(const Vector3Dd& point, double radius,
        const Matrix4x4d& forward, const Vector3Dd& localDirection);
};

#endif
