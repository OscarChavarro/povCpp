#ifndef __GEOMETRYINTERSECTIONEMISSIONCONTEXT__
#define __GEOMETRYINTERSECTIONEMISSIONCONTEXT__

class Material;
class RayCastingHitElement;

class GeometryIntersectionEmissionContext {
  public:
    GeometryIntersectionEmissionContext(
        Material *materialOverride,
        RayCastingHitElement *detailOwner,
        bool materialUsesObjectLocalPoint) :
        materialOverride(materialOverride),
        detailOwner(detailOwner),
        materialUsesObjectLocalPoint(materialUsesObjectLocalPoint)
    {}

    Material *getMaterialOverride() const { return materialOverride; }
    RayCastingHitElement *getDetailOwner() const { return detailOwner; }
    bool getMaterialUsesObjectLocalPoint() const { return materialUsesObjectLocalPoint; }

  private:
    Material * const materialOverride;
    RayCastingHitElement * const detailOwner;
    const bool materialUsesObjectLocalPoint;
};

#endif
