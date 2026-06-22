#ifndef __INTERSECTION_ATTRIBUTES__
#define __INTERSECTION_ATTRIBUTES__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/material/Material.h"
#include "environment/geometry/Geometry.h"

// Per-candidate attribution gathered while a ray is matched against the scene
// (which geometry/material produced the hit, and the shading overrides a
// containing BoundedGeometry applies) - kept apart from the candidate's own
// geometric data (Intersection: t/point/normal) so that class can stay a
// plain geometric record.
class IntersectionAttributes {
  private:
    Geometry *hitGeometry = nullptr;
    Material *material = nullptr;
    Material *objectTexture = nullptr;
    ColorRgba *objectColor = nullptr;
    bool noShadowFlag = false;

  public:
    Geometry *getHitGeometry() const;
    void setHitGeometry(Geometry *value);
    Material *getMaterial() const;
    void setMaterial(Material *value);
    Material *getObjectTexture() const;
    void setObjectTexture(Material *value);
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
