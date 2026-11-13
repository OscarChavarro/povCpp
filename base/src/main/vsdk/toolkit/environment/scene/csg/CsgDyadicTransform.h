#ifndef VSDK_CSG_DYADIC_TRANSFORM_H
#define VSDK_CSG_DYADIC_TRANSFORM_H
#include "vsdk/toolkit/environment/scene/csg/CsgDyadic.h"
class Matrix4x4d;
// Exact homogeneous inverse numerator; no intermediate binary64 rounding.
class CsgDyadicTransform {
public:
    static bool dot(const CsgDyadic* a, const CsgDyadic* b, int count, CsgDyadic& result);
    static bool inverseNumerator(const Matrix4x4d& forward, CsgDyadic result[4][4]);
};
#endif
