#ifndef VSDK_CSG_EXACT_TRANSFORM_H
#define VSDK_CSG_EXACT_TRANSFORM_H
class Matrix4x4d;
// Homogeneous inverse numerator det(A) * [A^-1, -A^-1 t; 0, 1].
class CsgExactTransform {
public:
    static bool exactSum(const double* a, const double* b, int count, double& result);
    static bool inverseNumerator(const Matrix4x4d& forward, double result[4][4]);
};
#endif
