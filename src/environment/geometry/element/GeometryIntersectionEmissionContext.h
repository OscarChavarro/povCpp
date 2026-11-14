#ifndef __GEOMETRYINTERSECTIONEMISSIONCONTEXT__
#define __GEOMETRYINTERSECTIONEMISSIONCONTEXT__

#include "vsdk/toolkit/environment/material/Material.h"
#include "environment/geometry/element/PostRayHitElement.h"

class GeometryIntersectionEmissionContext {
  public:
    GeometryIntersectionEmissionContext(
        Material *materialOverride,
        PostRayHitElement *detailOwner,
        bool materialUsesObjectLocalPoint) :
        materialOverride(materialOverride),
        detailOwner(detailOwner),
        materialUsesObjectLocalPoint(materialUsesObjectLocalPoint)
    {}

    Material *getMaterialOverride() const { return materialOverride; }
    PostRayHitElement *getDetailOwner() const { return detailOwner; }
    bool getMaterialUsesObjectLocalPoint() const { return materialUsesObjectLocalPoint; }

  private:
    Material * const materialOverride;
    PostRayHitElement * const detailOwner;
    const bool materialUsesObjectLocalPoint;
};

#endif
