#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"

AxisAlignedBoundingBox::AxisAlignedBoundingBox() : min(0.0, 0.0, 0.0), max(0.0, 0.0, 0.0)
{
}

AxisAlignedBoundingBox::AxisAlignedBoundingBox(const Vector3Dd &min, const Vector3Dd &max) :
    min(min), max(max)
{
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::unbounded()
{
    constexpr double INF = 1e30;
    return AxisAlignedBoundingBox(Vector3Dd(-INF, -INF, -INF), Vector3Dd(INF, INF, INF));
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::empty()
{
    constexpr double INF = 1e30;
    return AxisAlignedBoundingBox(Vector3Dd(INF, INF, INF), Vector3Dd(-INF, -INF, -INF));
}

bool AxisAlignedBoundingBox::isUnbounded() const
{
    constexpr double THRESHOLD = 1e29;
    return min.x() <= -THRESHOLD || max.x() >= THRESHOLD
        || min.y() <= -THRESHOLD || max.y() >= THRESHOLD
        || min.z() <= -THRESHOLD || max.z() >= THRESHOLD;
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::enclosing(const AxisAlignedBoundingBox &other) const
{
    return AxisAlignedBoundingBox(
        Vector3Dd(
            min.x() < other.min.x() ? min.x() : other.min.x(),
            min.y() < other.min.y() ? min.y() : other.min.y(),
            min.z() < other.min.z() ? min.z() : other.min.z()),
        Vector3Dd(
            max.x() > other.max.x() ? max.x() : other.max.x(),
            max.y() > other.max.y() ? max.y() : other.max.y(),
            max.z() > other.max.z() ? max.z() : other.max.z()));
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::intersection(const AxisAlignedBoundingBox &other) const
{
    return AxisAlignedBoundingBox(
        Vector3Dd(
            min.x() > other.min.x() ? min.x() : other.min.x(),
            min.y() > other.min.y() ? min.y() : other.min.y(),
            min.z() > other.min.z() ? min.z() : other.min.z()),
        Vector3Dd(
            max.x() < other.max.x() ? max.x() : other.max.x(),
            max.y() < other.max.y() ? max.y() : other.max.y(),
            max.z() < other.max.z() ? max.z() : other.max.z()));
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::expandedBy(const Vector3Dd &point) const
{
    return AxisAlignedBoundingBox(
        Vector3Dd(
            min.x() < point.x() ? min.x() : point.x(),
            min.y() < point.y() ? min.y() : point.y(),
            min.z() < point.z() ? min.z() : point.z()),
        Vector3Dd(
            max.x() > point.x() ? max.x() : point.x(),
            max.y() > point.y() ? max.y() : point.y(),
            max.z() > point.z() ? max.z() : point.z()));
}

Vector3Dd AxisAlignedBoundingBox::centroid() const
{
    return Vector3Dd(
        (min.x() + max.x()) * 0.5,
        (min.y() + max.y()) * 0.5,
        (min.z() + max.z()) * 0.5);
}

double AxisAlignedBoundingBox::extent(int axis) const
{
    if (axis == 0) return max.x() - min.x();
    if (axis == 1) return max.y() - min.y();
    return max.z() - min.z();
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::fromTransformedCorners(
    const Vector3Dd &lo, const Vector3Dd &hi, const Matrix4x4d *matrix)
{
    GeometryBounds local;
    if (!local.setFinite(lo, hi)) return unbounded();
    const GeometryBounds result = matrix ? local.transformed(*matrix) : local;
    if (result.getKind() != GeometryBounds::Kind::Finite) return unbounded();
    return AxisAlignedBoundingBox(result.getMinimum(), result.getMaximum());
}

AxisAlignedBoundingBox AxisAlignedBoundingBox::fromGeometryBounds(const GeometryBounds &bounds)
{
    if (bounds.getKind() != GeometryBounds::Kind::Finite) return unbounded();
    return AxisAlignedBoundingBox(bounds.getMinimum(), bounds.getMaximum());
}
