#ifndef VSDK_AFFINE_TOLERANCE_MAP_H
#define VSDK_AFFINE_TOLERANCE_MAP_H
#include "vsdk/toolkit/environment/scene/AffinePlacementStatus.h"
class Matrix4x4d;
class Vector3Dd;
// Enclose the inverse image of a parent-space tolerance ball in a child-space
// ball and, when requested, provide a child ball enclosed in that inverse image.
// A negative inner radius means no positive inner ball was certified.
// childHalfWidths, when requested, bounds the same inverse image by an
// axis-aligned child box, which is tighter under anisotropic placement.
class AffineToleranceMap {
public:
    static AffinePlacementStatus map(const Matrix4x4d& forward, const Vector3Dd& point,
        double tolerance, Vector3Dd& childPoint, double& childTolerance,
        double* childInnerTolerance = nullptr, Vector3Dd* childHalfWidths = nullptr);
};
#endif
