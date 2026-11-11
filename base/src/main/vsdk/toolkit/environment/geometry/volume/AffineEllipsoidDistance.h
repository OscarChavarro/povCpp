#ifndef __AFFINE_ELLIPSOID_DISTANCE__
#define __AFFINE_ELLIPSOID_DISTANCE__

#include "vsdk/toolkit/environment/geometry/element/GeometryDistanceStatus.h"
class Matrix4x4d;
class Vector3Dd;

// Estimates Euclidean distance to an affine sphere, including shear. Returns
// an explicit status and an estimated (not formally proven) numerical guard.
// Outputs are assigned only on Valid. Do not treat the estimate as a certified
// interval or use it to force a CSG classification near the tolerance band.
class AffineEllipsoidDistance {
public:
    static GeometryDistanceStatus query(const Vector3Dd& point, double radius,
        const Matrix4x4d& forward, double& distance, double& estimatedError);
    static GeometryDistanceStatus queryWithWitness(const Vector3Dd& point, double radius,
        const Matrix4x4d& forward, double& distance, double& estimatedError, Vector3Dd& witness);
};

#endif
