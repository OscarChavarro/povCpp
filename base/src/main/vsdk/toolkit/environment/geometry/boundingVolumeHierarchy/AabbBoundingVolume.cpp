#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AabbBoundingVolume.h"

AabbBoundingVolume::AabbBoundingVolume() : bounds(AxisAlignedBoundingBox::unbounded())
{
}

AabbBoundingVolume::AabbBoundingVolume(const AxisAlignedBoundingBox &bounds) : bounds(bounds)
{
}

AabbBoundingVolume::AabbBoundingVolume(const Vector3Dd &min, const Vector3Dd &max) :
    bounds(min, max)
{
}

bool AabbBoundingVolume::mayIntersect(const Ray &ray) const
{
    return bounds.intersectsRayForward(ray);
}

bool AabbBoundingVolume::mayIntersectBefore(const Ray &ray, double maxT) const
{
    return bounds.intersectsRayBefore(ray, maxT);
}

bool AabbBoundingVolume::containsPoint(const Vector3Dd &point, double tolerance) const
{
    return bounds.containsPointWithTolerance(point, tolerance);
}

bool AabbBoundingVolume::isUnbounded() const
{
    return bounds.isUnbounded();
}

AxisAlignedBoundingBox AabbBoundingVolume::axisAlignedExtent() const
{
    return bounds;
}

BoundingVolumeHierarchy *AabbBoundingVolume::copy() const
{
    return new AabbBoundingVolume(bounds);
}
