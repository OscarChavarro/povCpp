#ifndef __BOUNDING_VOLUME_HIERARCHY__
#define __BOUNDING_VOLUME_HIERARCHY__

#include "environment/geometry/element/PreRayHitElement.h"

class AxisAlignedBoundingBox;

class BoundingVolumeHierarchy : public PreRayHitElement {
  public:
    virtual ~BoundingVolumeHierarchy() {};

    // Conservative queries: false positives are allowed; false negatives are not.
    virtual bool containsPoint(const Vector3Dd &point, double tolerance) const = 0;
    virtual bool isUnbounded() const = 0;

    // Conservative axis-aligned extent of the whole volume.
    virtual AxisAlignedBoundingBox axisAlignedExtent() const = 0;

    // Caller owns the returned copy.
    virtual BoundingVolumeHierarchy *copy() const = 0;
};

#endif
