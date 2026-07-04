#ifndef __AXIS_ALIGNED_BOUNDING_BOX__
#define __AXIS_ALIGNED_BOUNDING_BOX__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

struct AxisAlignedBoundingBox {
    Vector3Dd min;
    Vector3Dd max;

    static AxisAlignedBoundingBox unbounded() {
        constexpr double INF = 1e30;
        return AxisAlignedBoundingBox{
            Vector3Dd(-INF, -INF, -INF),
            Vector3Dd( INF,  INF,  INF)};
    }

    static AxisAlignedBoundingBox empty() {
        constexpr double INF = 1e30;
        return AxisAlignedBoundingBox{
            Vector3Dd( INF,  INF,  INF),
            Vector3Dd(-INF, -INF, -INF)};
    }

    bool isUnbounded() const {
        constexpr double THRESHOLD = 1e29;
        return min.x() <= -THRESHOLD || max.x() >= THRESHOLD
            || min.y() <= -THRESHOLD || max.y() >= THRESHOLD
            || min.z() <= -THRESHOLD || max.z() >= THRESHOLD;
    }

    AxisAlignedBoundingBox enclosing(const AxisAlignedBoundingBox &other) const {
        return AxisAlignedBoundingBox{
            Vector3Dd(
                min.x() < other.min.x() ? min.x() : other.min.x(),
                min.y() < other.min.y() ? min.y() : other.min.y(),
                min.z() < other.min.z() ? min.z() : other.min.z()),
            Vector3Dd(
                max.x() > other.max.x() ? max.x() : other.max.x(),
                max.y() > other.max.y() ? max.y() : other.max.y(),
                max.z() > other.max.z() ? max.z() : other.max.z())};
    }

    AxisAlignedBoundingBox intersection(const AxisAlignedBoundingBox &other) const {
        return AxisAlignedBoundingBox{
            Vector3Dd(
                min.x() > other.min.x() ? min.x() : other.min.x(),
                min.y() > other.min.y() ? min.y() : other.min.y(),
                min.z() > other.min.z() ? min.z() : other.min.z()),
            Vector3Dd(
                max.x() < other.max.x() ? max.x() : other.max.x(),
                max.y() < other.max.y() ? max.y() : other.max.y(),
                max.z() < other.max.z() ? max.z() : other.max.z())};
    }

    AxisAlignedBoundingBox expandedBy(const Vector3Dd &point) const {
        return AxisAlignedBoundingBox{
            Vector3Dd(
                min.x() < point.x() ? min.x() : point.x(),
                min.y() < point.y() ? min.y() : point.y(),
                min.z() < point.z() ? min.z() : point.z()),
            Vector3Dd(
                max.x() > point.x() ? max.x() : point.x(),
                max.y() > point.y() ? max.y() : point.y(),
                max.z() > point.z() ? max.z() : point.z())};
    }

    Vector3Dd centroid() const {
        return Vector3Dd(
            (min.x() + max.x()) * 0.5,
            (min.y() + max.y()) * 0.5,
            (min.z() + max.z()) * 0.5);
    }

    double extent(int axis) const {
        if (axis == 0) return max.x() - min.x();
        if (axis == 1) return max.y() - min.y();
        return max.z() - min.z();
    }

    // Build world-space AABB by transforming 8 corners through matrix.
    // If matrix is null, returns an AABB around [lo, hi] directly.
    static AxisAlignedBoundingBox fromTransformedCorners(
        const Vector3Dd &lo, const Vector3Dd &hi, const Matrix4x4d *matrix)
    {
        AxisAlignedBoundingBox result = empty();
        for (int ix = 0; ix < 2; ix++) {
            double x = (ix == 0) ? lo.x() : hi.x();
            for (int iy = 0; iy < 2; iy++) {
                double y = (iy == 0) ? lo.y() : hi.y();
                for (int iz = 0; iz < 2; iz++) {
                    double z = (iz == 0) ? lo.z() : hi.z();
                    Vector3Dd corner = matrix
                        ? matrix->transformPoint(Vector3Dd(x, y, z))
                        : Vector3Dd(x, y, z);
                    result = result.expandedBy(corner);
                }
            }
        }
        return result;
    }
};

#endif
