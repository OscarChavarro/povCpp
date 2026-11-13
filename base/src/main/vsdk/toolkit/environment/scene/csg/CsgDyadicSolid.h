#ifndef VSDK_CSG_DYADIC_SOLID_H
#define VSDK_CSG_DYADIC_SOLID_H
#include "vsdk/toolkit/environment/scene/csg/CsgDyadic.h"
class Matrix4x4d;
// Exact sufficient certificates; failure never establishes non-equivalence.
class CsgDyadicSolid {
private:
    static bool proportional(const CsgDyadic* a, const CsgDyadic* b, int count);
    static bool planeWorld(const double* local, const Matrix4x4d& forward, CsgDyadic* result);
    static bool quadricWorld(const CsgDyadic* local, const Matrix4x4d& forward, CsgDyadic* result);
    static bool quadricLocal(const double* local, CsgDyadic* result);
public:
    static bool samePlanes(const double* a, const Matrix4x4d& pa, const double* b, const Matrix4x4d& pb);
    static bool sameQuadrics(const double* a, const Matrix4x4d& pa, const double* b, const Matrix4x4d& pb);
    static bool sphereQuadric(double radius, const Matrix4x4d& ps, const double* q, const Matrix4x4d& pq);
    static bool sameEllipsoids(double ra, const Matrix4x4d& pa, double rb, const Matrix4x4d& pb);
};
#endif
