#ifndef __INTERSECTION_ATTRIBUTES__
#define __INTERSECTION_ATTRIBUTES__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/geometry/Geometry.h"

// Per-candidate attribution gathered while a ray is matched against the scene
// (which geometry/material produced the hit, and the shading overrides a
// containing scene object applies) - kept apart from the candidate's own
// geometric data (Intersection: t/point/normal) so that class can stay a
// plain geometric record.
class IntersectionAttributes {
  private:
    static constexpr int MAX_DETAIL_OWNERS = 8;

    Geometry *hitGeometry = nullptr;
    RayOperationOwner *hitBody = nullptr;
    RayOperationOwner *detailOwners[MAX_DETAIL_OWNERS] = {};
    int detailOwnerCount = 0;
    Material *material = nullptr;
    Material *objectTexture = nullptr;
    ColorRgba *objectColor = nullptr;
    bool noShadowFlag = false;
    bool materialUsesObjectLocalPoint = false;

  public:
    Geometry *getHitGeometry() const;
    void setHitGeometry(Geometry *value);
    RayOperationOwner *getHitBody() const;
    void setHitBody(RayOperationOwner *value);
    int getDetailOwnerCount() const;
    RayOperationOwner *getDetailOwnerAt(int index) const;
    void clearDetailOwners();
    void pushDetailOwner(RayOperationOwner *value);
    void prependDetailOwner(RayOperationOwner *value);
    Material *getMaterial() const;
    void setMaterial(Material *value);
    Material *getObjectTexture() const;
    void setObjectTexture(Material *value);
    bool getMaterialUsesObjectLocalPoint() const;
    void setMaterialUsesObjectLocalPoint(bool value);
    ColorRgba *getObjectColor() const;
    void setObjectColor(ColorRgba *value);
    bool getNoShadowFlag() const;
    void setNoShadowFlag(bool value);
};

inline Geometry *
IntersectionAttributes::getHitGeometry() const
{
    return hitGeometry;
}

inline void
IntersectionAttributes::setHitGeometry(Geometry *value)
{
    hitGeometry = value;
}

inline RayOperationOwner *
IntersectionAttributes::getHitBody() const
{
    return hitBody;
}

inline void
IntersectionAttributes::setHitBody(RayOperationOwner *value)
{
    hitBody = value;
}

inline int
IntersectionAttributes::getDetailOwnerCount() const
{
    return detailOwnerCount;
}

inline void
IntersectionAttributes::clearDetailOwners()
{
    detailOwnerCount = 0;
}

inline RayOperationOwner *
IntersectionAttributes::getDetailOwnerAt(int index) const
{
    if (index < 0 || index >= detailOwnerCount) {
        return nullptr;
    }
    return detailOwners[index];
}

inline void
IntersectionAttributes::pushDetailOwner(RayOperationOwner *value)
{
    if (value == nullptr || detailOwnerCount >= MAX_DETAIL_OWNERS) {
        return;
    }
    detailOwners[detailOwnerCount++] = value;
}

inline void
IntersectionAttributes::prependDetailOwner(RayOperationOwner *value)
{
    if (value == nullptr || detailOwnerCount >= MAX_DETAIL_OWNERS) {
        return;
    }
    for (int i = detailOwnerCount; i > 0; --i) {
        detailOwners[i] = detailOwners[i - 1];
    }
    detailOwners[0] = value;
    detailOwnerCount++;
}

inline Material *
IntersectionAttributes::getMaterial() const
{
    return material;
}

inline void
IntersectionAttributes::setMaterial(Material *value)
{
    material = value;
}

inline Material *
IntersectionAttributes::getObjectTexture() const
{
    return objectTexture;
}

inline void
IntersectionAttributes::setObjectTexture(Material *value)
{
    objectTexture = value;
}

inline bool
IntersectionAttributes::getMaterialUsesObjectLocalPoint() const
{
    return materialUsesObjectLocalPoint;
}

inline void
IntersectionAttributes::setMaterialUsesObjectLocalPoint(bool value)
{
    materialUsesObjectLocalPoint = value;
}

inline ColorRgba *
IntersectionAttributes::getObjectColor() const
{
    return objectColor;
}

inline void
IntersectionAttributes::setObjectColor(ColorRgba *value)
{
    objectColor = value;
}

inline bool
IntersectionAttributes::getNoShadowFlag() const
{
    return noShadowFlag;
}

inline void
IntersectionAttributes::setNoShadowFlag(bool value)
{
    noShadowFlag = value;
}

#endif
