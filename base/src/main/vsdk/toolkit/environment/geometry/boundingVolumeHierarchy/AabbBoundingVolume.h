#ifndef __AABB_BOUNDING_VOLUME__
#define __AABB_BOUNDING_VOLUME__

#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/AxisAlignedBoundingBox.h"
#include "vsdk/toolkit/environment/geometry/boundingVolumeHierarchy/BoundingVolumeHierarchy.h"

/** Bounding volume made of one axis aligned box. */
class AabbBoundingVolume final : public BoundingVolumeHierarchy {
  private:
    AxisAlignedBoundingBox bounds;

  public:
    AabbBoundingVolume();
    explicit AabbBoundingVolume(const AxisAlignedBoundingBox &bounds);
    AabbBoundingVolume(const Vector3Dd &min, const Vector3Dd &max);

    const AxisAlignedBoundingBox &getBounds() const;

    bool mayIntersect(const Ray &ray) const override;
    bool mayIntersectBefore(const Ray &ray, double maxT) const override;
    bool containsPoint(const Vector3Dd &point, double tolerance) const override;
    bool isUnbounded() const override;
    AxisAlignedBoundingBox axisAlignedExtent() const override;
    BoundingVolumeHierarchy *copy() const override;
};

inline const AxisAlignedBoundingBox &AabbBoundingVolume::getBounds() const
{
    return bounds;
}

#endif
