#ifndef __AABB_BOUNDING_VOLUME__
#define __AABB_BOUNDING_VOLUME__

#include "environment/geometry/boundingVolumeHierarchy/AxisAlignedBoundingBox.h"
#include "environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h"

class AabbBoundingVolume final : public BoundingVolumeHierarchy {
  private:
    AxisAlignedBoundingBox bounds;

  public:
    AabbBoundingVolume() : bounds(AxisAlignedBoundingBox::unbounded()) {}
    explicit AabbBoundingVolume(const AxisAlignedBoundingBox &bounds) : bounds(bounds) {}
    AabbBoundingVolume(const Vector3Dd &min, const Vector3Dd &max) : bounds(min, max) {}

    const AxisAlignedBoundingBox &getBounds() const { return bounds; }

    bool mayIntersect(const RayWithTracingState &ray) const override
    {
        return bounds.intersectsRayForward(ray);
    }

    bool mayIntersectBefore(const RayWithTracingState &ray, double maxT) const override
    {
        return bounds.intersectsRayBefore(ray, maxT);
    }

    bool containsPoint(const Vector3Dd &point, double tolerance) const override
    {
        return bounds.containsPointWithTolerance(point, tolerance);
    }

    bool isUnbounded() const override
    {
        return bounds.isUnbounded();
    }

    AxisAlignedBoundingBox axisAlignedExtent() const override
    {
        return bounds;
    }

    BoundingVolumeHierarchy *copy() const override
    {
        return new AabbBoundingVolume(bounds);
    }
};

#endif
