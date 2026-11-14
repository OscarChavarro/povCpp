#ifndef __AXIS_ALIGNED_BOUNDING_BOX__
#define __AXIS_ALIGNED_BOUNDING_BOX__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/RaySlabReciprocals.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"

class GeometryBounds;

/**
Axis aligned box `[min, max]`. Coordinates beyond `1e29` stand for an
unbounded box. The ray tests are the slab method of Kay and Kajiya (1986);
they and the point test are the hottest filters of a render, so their
definitions are inline in this header.
*/
class AxisAlignedBoundingBox {
  private:
    Vector3Dd min;
    Vector3Dd max;

  public:
    AxisAlignedBoundingBox();
    AxisAlignedBoundingBox(const Vector3Dd &min, const Vector3Dd &max);

    const Vector3Dd &getMin() const;
    const Vector3Dd &getMax() const;

    static AxisAlignedBoundingBox unbounded();
    static AxisAlignedBoundingBox empty();
    bool isUnbounded() const;

    AxisAlignedBoundingBox enclosing(const AxisAlignedBoundingBox &other) const;
    AxisAlignedBoundingBox intersection(const AxisAlignedBoundingBox &other) const;
    AxisAlignedBoundingBox expandedBy(const Vector3Dd &point) const;
    Vector3Dd centroid() const;
    double extent(int axis) const;

    /** World box around the eight corners of `[lo, hi]` transformed by
    `matrix` (column vectors, as `GeometryBounds::transformed`); without a
    matrix, the box `[lo, hi]` itself. */
    static AxisAlignedBoundingBox fromTransformedCorners(
        const Vector3Dd &lo, const Vector3Dd &hi, const Matrix4x4d *matrix);
    /** Box of finite geometry bounds; any other kind (empty, unbounded,
    indeterminate, unsupported) gives the unbounded box, which is always
    conservative. */
    static AxisAlignedBoundingBox fromGeometryBounds(const GeometryBounds &bounds);

    /** Slab test for ray parameters in [0, +inf). */
    bool intersectsRayForward(const Ray &ray) const;
    /** Slab test for ray parameters in [0, +inf), with the reciprocals of
    the ray direction computed beforehand. */
    bool intersectsRayForward(const Ray &ray, const RaySlabReciprocals &reciprocals) const;
    /** Slab test for ray parameters in [0, maxT]. */
    bool intersectsRayBefore(const Ray &ray, double maxT) const;

    bool containsPointWithTolerance(const Vector3Dd &point, double tolerance) const;
};

inline const Vector3Dd &AxisAlignedBoundingBox::getMin() const
{
    return min;
}

inline const Vector3Dd &AxisAlignedBoundingBox::getMax() const
{
    return max;
}

inline bool AxisAlignedBoundingBox::intersectsRayForward(const Ray &ray) const
{
    return intersectsRayForward(ray, RaySlabReciprocals(ray.getDirection()));
}

inline bool AxisAlignedBoundingBox::intersectsRayForward(
    const Ray &ray, const RaySlabReciprocals &reciprocals) const
{
    const Vector3Dd origin = ray.getOrigin();
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
        updateAxis(origin.x(), reciprocals.getInverseX(), reciprocals.isDegenerateX(),
                   min.x(), max.x()) &&
        updateAxis(origin.y(), reciprocals.getInverseY(), reciprocals.isDegenerateY(),
                   min.y(), max.y()) &&
        updateAxis(origin.z(), reciprocals.getInverseZ(), reciprocals.isDegenerateZ(),
                   min.z(), max.z()) &&
        tMax >= 0.0;
}

inline bool AxisAlignedBoundingBox::intersectsRayBefore(const Ray &ray, double maxT) const
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

inline bool AxisAlignedBoundingBox::containsPointWithTolerance(const Vector3Dd &point, double tolerance) const
{
    return
        point.x() >= min.x() - tolerance &&
        point.x() <= max.x() + tolerance &&
        point.y() >= min.y() - tolerance &&
        point.y() <= max.y() + tolerance &&
        point.z() >= min.z() - tolerance &&
        point.z() <= max.z() + tolerance;
}

#endif
