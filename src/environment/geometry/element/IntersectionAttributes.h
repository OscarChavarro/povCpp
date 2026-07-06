#ifndef __INTERSECTION_ATTRIBUTES__
#define __INTERSECTION_ATTRIBUTES__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/material/Material.h"
#include "environment/geometry/element/RayCastingHitElement.h"

// Per-candidate attribution gathered while a ray is matched against the scene
// (which geometry/material produced the hit, and the shading overrides a
// containing scene object applies) - kept apart from the candidate's own
// geometric data (Intersection: t/point/normal) so that class can stay a
// plain geometric record.
//
// hitGeometry and hitBody are both RayCastingHitElement* - Geometry implements
// that interface precisely so a bare shape (no wrapping body) can travel
// through the same pointer type as a CSG body, without this class (or
// anything in element/) needing to know the concrete Geometry class. They
// are NOT interchangeable: a body's doExtraInformation may consult hitGeometry
// on its own account (e.g. a baked/quadric override), so both must be kept as
// distinct fields even though their type is now the same.
class IntersectionAttributes {
  private:
    static constexpr int MAX_DETAIL_OWNERS = 8;

    RayCastingHitElement *hitGeometry = nullptr;
    RayCastingHitElement *hitBody = nullptr;
    RayCastingHitElement *detailOwners[MAX_DETAIL_OWNERS] = {};
    int detailOwnerCount = 0;
    Material *material = nullptr;
    Material *objectTexture = nullptr;
    ColorRgba *objectColor = nullptr;
    bool noShadowFlag = false;
    bool materialUsesObjectLocalPoint = false;

  public:
    RayCastingHitElement *getHitGeometry() const;
    void setHitGeometry(RayCastingHitElement *value);
    RayCastingHitElement *getHitBody() const;
    void setHitBody(RayCastingHitElement *value);
    int getDetailOwnerCount() const;
    RayCastingHitElement *getDetailOwnerAt(int index) const;
    void pushDetailOwner(RayCastingHitElement *value);
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

inline RayCastingHitElement *
IntersectionAttributes::getHitGeometry() const
{
    return hitGeometry;
}

inline void
IntersectionAttributes::setHitGeometry(RayCastingHitElement *value)
{
    hitGeometry = value;
}

inline RayCastingHitElement *
IntersectionAttributes::getHitBody() const
{
    return hitBody;
}

inline void
IntersectionAttributes::setHitBody(RayCastingHitElement *value)
{
    hitBody = value;
}

inline int
IntersectionAttributes::getDetailOwnerCount() const
{
    return detailOwnerCount;
}

inline RayCastingHitElement *
IntersectionAttributes::getDetailOwnerAt(int index) const
{
    if (index < 0 || index >= detailOwnerCount) {
        return nullptr;
    }
    return detailOwners[index];
}

inline void
IntersectionAttributes::pushDetailOwner(RayCastingHitElement *value)
{
    if (value == nullptr || detailOwnerCount >= MAX_DETAIL_OWNERS) {
        return;
    }
    detailOwners[detailOwnerCount++] = value;
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
