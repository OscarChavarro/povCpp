#ifndef VSDK_CSG_EXACT_QUADRIC_H
#define VSDK_CSG_EXACT_QUADRIC_H
class Matrix4x4d;
// Coefficient order: xx, yy, zz, xy, xz, yz, x, y, z, constant.
class CsgExactQuadric {
public:
    static bool world(const double* local, const Matrix4x4d& forward, double* result);
};
#endif
