#ifndef __INTERSECTION_ATTRIBUTES__
#define __INTERSECTION_ATTRIBUTES__

#include <stdexcept>
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/environment/material/Material.h"
#include "vsdk/toolkit/environment/geometry/Geometry.h"
#include "environment/geometry/element/PostRayHitElement.h"
#include "environment/geometry/element/SurfaceDetailFamily.h"

// Per-candidate attribution gathered while a ray is matched against the scene,
// kept apart from the candidate's geometric data (Intersection: t/point/normal).
// The hit is identified by the VITRAL surface it lies on and that surface's
// detail family (resolved by SurfaceDetailKernel), by the scene body that owns
// it and by the chain of detail owners (CSG operands, composites) between
// them; plus the shading overrides a containing scene object applies.
class IntersectionAttributes {
  private:
    static constexpr int MAX_DETAIL_OWNERS = 8;

    Geometry *hitSurface = nullptr;
    PostRayHitElement *hitBody = nullptr;
    PostRayHitElement *detailOwners[MAX_DETAIL_OWNERS] = {};
    int detailOwnerCount = 0;
    int surfaceIndex = -1;
    Material *material = nullptr;
    Material *objectTexture = nullptr;
    ColorRgba *objectColor = nullptr;
    SurfaceDetailFamily hitSurfaceFamily = SurfaceDetailFamily::None;
    bool noShadowFlag = false;
    bool materialUsesObjectLocalPoint = false;

  public:
    Geometry *getHitSurface() const;
    SurfaceDetailFamily getHitSurfaceFamily() const;
    void setHitSurface(Geometry *surface, SurfaceDetailFamily family);
    PostRayHitElement *getHitBody() const;
    void setHitBody(PostRayHitElement *value);
    int getSurfaceIndex() const;
    void setSurfaceIndex(int value);
    int getDetailOwnerCount() const;
    PostRayHitElement *getDetailOwnerAt(int index) const;
    void pushDetailOwner(PostRayHitElement *value);
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
IntersectionAttributes::getHitSurface() const
{
    return hitSurface;
}

inline SurfaceDetailFamily
IntersectionAttributes::getHitSurfaceFamily() const
{
    return hitSurfaceFamily;
}

inline void
IntersectionAttributes::setHitSurface(Geometry *surface, SurfaceDetailFamily family)
{
    hitSurface = surface;
    hitSurfaceFamily = family;
}

inline PostRayHitElement *
IntersectionAttributes::getHitBody() const
{
    return hitBody;
}

inline void
IntersectionAttributes::setHitBody(PostRayHitElement *value)
{
    hitBody = value;
}

inline int IntersectionAttributes::getSurfaceIndex() const { return surfaceIndex; }
inline void IntersectionAttributes::setSurfaceIndex(int value) { surfaceIndex = value; }

inline int
IntersectionAttributes::getDetailOwnerCount() const
{
    return detailOwnerCount;
}

inline PostRayHitElement *
IntersectionAttributes::getDetailOwnerAt(int index) const
{
    if (index < 0 || index >= detailOwnerCount) {
        return nullptr;
    }
    return detailOwners[index];
}

inline void
IntersectionAttributes::pushDetailOwner(PostRayHitElement *value)
{
    if (value == nullptr) return;
    if (detailOwnerCount >= MAX_DETAIL_OWNERS) {
        throw std::overflow_error("Ray-casting detail owner chain exceeds 8 levels");
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
