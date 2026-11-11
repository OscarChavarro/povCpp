#ifndef __ORTHOGONAL_ELLIPSOID_DISTANCE__
#define __ORTHOGONAL_ELLIPSOID_DISTANCE__

#include "vsdk/toolkit/environment/geometry/element/GeometryDistanceStatus.h"
class Matrix4x4d;
class Vector3Dd;

// Euclidean distance to an affine sphere whose three transformed axes are
// orthogonal. The error estimate is a numerical guard, not a proven bound.
class OrthogonalEllipsoidDistance {
public:
    static GeometryDistanceStatus query(const Vector3Dd& point, double radius, const Matrix4x4d& forward,
        double& distance, double& estimatedError);
    static GeometryDistanceStatus queryWithWitness(const Vector3Dd& point, double radius,
        const Matrix4x4d& forward, double& distance, double& estimatedError, Vector3Dd& witness);
};

#endif
