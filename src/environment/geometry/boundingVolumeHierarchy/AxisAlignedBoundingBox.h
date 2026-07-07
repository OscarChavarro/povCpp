#ifndef __AXIS_ALIGNED_BOUNDING_BOX__
#define __AXIS_ALIGNED_BOUNDING_BOX__

#include "environment/geometry/element/RayWithTracingState.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"

class AxisAlignedBoundingBox {
  private:
    Vector3Dd min;
    Vector3Dd max;

  public:
    AxisAlignedBoundingBox() : min(0.0, 0.0, 0.0), max(0.0, 0.0, 0.0) {}
    AxisAlignedBoundingBox(const Vector3Dd &min, const Vector3Dd &max) : min(min), max(max) {}

    const Vector3Dd &getMin() const { return min; }
    const Vector3Dd &getMax() const { return max; }

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

    bool intersectsRayForward(const RayWithTracingState &ray) const
    {
        const Vector3Dd origin = ray.getOrigin();
        double invDirX, invDirY, invDirZ;
        bool degenerateX, degenerateY, degenerateZ;
        ray.getAabbSlabReciprocals(
            &invDirX, &invDirY, &invDirZ,
            &degenerateX, &degenerateY, &degenerateZ);
        double tMin = 0.0;
        double tMax = 1e30;

        auto updateAxis = [&](double originCoord, double invDir, bool degenerate,
                              double minCoord, double maxCoord) -> bool {
            if (degenerate) {
                return originCoord >= minCoord && originCoord <= maxCoord;
            }
            double nearT = (minCoord - originCoord) * invDir;
            double farT = (maxCoord - originCoord) * invDir;
            if (nearT > farT) {
                const double tmp = nearT;
                nearT = farT;
                farT = tmp;
            }
            tMin = nearT > tMin ? nearT : tMin;
            tMax = farT < tMax ? farT : tMax;
            return tMin <= tMax;
        };

        return
            updateAxis(origin.x(), invDirX, degenerateX, min.x(), max.x()) &&
            updateAxis(origin.y(), invDirY, degenerateY, min.y(), max.y()) &&
            updateAxis(origin.z(), invDirZ, degenerateZ, min.z(), max.z()) &&
            tMax >= 0.0;
    }

    bool intersectsRayBefore(const RayWithTracingState &ray, double maxT) const
    {
        const Vector3Dd origin = ray.getOrigin();
        const Vector3Dd direction = ray.getDirection();
        double tMin = 0.0;
        double tMax = maxT;

        auto updateAxis = [&](double originCoord, double directionCoord,
                              double minCoord, double maxCoord) -> bool {
            if (directionCoord > -1e-12 && directionCoord < 1e-12) {
                return originCoord >= minCoord && originCoord <= maxCoord;
            }
            const double invDir = 1.0 / directionCoord;
            double nearT = (minCoord - originCoord) * invDir;
            double farT = (maxCoord - originCoord) * invDir;
            if (nearT > farT) {
                const double tmp = nearT;
                nearT = farT;
                farT = tmp;
            }
            tMin = nearT > tMin ? nearT : tMin;
            tMax = farT < tMax ? farT : tMax;
            return tMin <= tMax;
        };

        return
            updateAxis(origin.x(), direction.x(), min.x(), max.x()) &&
            updateAxis(origin.y(), direction.y(), min.y(), max.y()) &&
            updateAxis(origin.z(), direction.z(), min.z(), max.z()) &&
            tMax >= 0.0;
    }

    bool containsPointWithTolerance(const Vector3Dd &point, double tolerance) const
    {
        return
            point.x() >= min.x() - tolerance &&
            point.x() <= max.x() + tolerance &&
            point.y() >= min.y() - tolerance &&
            point.y() <= max.y() + tolerance &&
            point.z() >= min.z() - tolerance &&
            point.z() <= max.z() + tolerance;
    }

};

#endif
