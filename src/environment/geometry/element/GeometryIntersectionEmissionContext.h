#ifndef __GEOMETRYINTERSECTIONEMISSIONCONTEXT__
#define __GEOMETRYINTERSECTIONEMISSIONCONTEXT__

class Material;
class RayOperationOwner;

class GeometryIntersectionEmissionContext {
  public:
    GeometryIntersectionEmissionContext(
        Material *materialOverride,
        RayOperationOwner *detailOwner,
        bool materialUsesObjectLocalPoint) :
        materialOverride(materialOverride),
        detailOwner(detailOwner),
        materialUsesObjectLocalPoint(materialUsesObjectLocalPoint)
    {}

    Material *getMaterialOverride() const { return materialOverride; }
    RayOperationOwner *getDetailOwner() const { return detailOwner; }
    bool getMaterialUsesObjectLocalPoint() const { return materialUsesObjectLocalPoint; }

  private:
    Material * const materialOverride;
    RayOperationOwner * const detailOwner;
    const bool materialUsesObjectLocalPoint;
};

#endif
