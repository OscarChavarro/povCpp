#ifndef __INTERSECTION__
#define __INTERSECTION__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "java/util/PriorityQueue.h"
#include "environment/material/Material.h"
#include "environment/geometry/Geometry.h"

class Intersection {
  private:
    double t;
    Vector3Dd point;
    Vector3Dd normal;

    Geometry *hitGeometry = nullptr;
    Material *material = nullptr;
    Material *objectTexture = nullptr;
    ColorRgba *objectColor = nullptr;
    bool noShadowFlag = false;

  public:
    double getT() const;
    void setT(double value);
    const Vector3Dd& getPoint() const;
    void setPoint(const Vector3Dd &value);
    const Vector3Dd& getNormal() const;
    void setNormal(const Vector3Dd &value);

    Vector3Dd& getPoint();
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

inline double
Intersection::getT() const
{
    return t;
}

inline void
Intersection::setT(double value)
{
    t = value;
}

inline const Vector3Dd&
Intersection::getPoint() const
{
    return point;
}

inline void
Intersection::setPoint(const Vector3Dd &value)
{
    point = value;
}

inline const Vector3Dd&
Intersection::getNormal() const
{
    return normal;
}

inline void
Intersection::setNormal(const Vector3Dd &value)
{
    normal = value;
}

inline Vector3Dd&
Intersection::getPoint()
{
    return point;
}

inline Geometry *
Intersection::getHitGeometry() const
{
    return hitGeometry;
}

inline void
Intersection::setHitGeometry(Geometry *value)
{
    hitGeometry = value;
}

inline Material *
Intersection::getMaterial() const
{
    return material;
}

inline void
Intersection::setMaterial(Material *value)
{
    material = value;
}

inline Material *
Intersection::getObjectTexture() const
{
    return objectTexture;
}

inline void
Intersection::setObjectTexture(Material *value)
{
    objectTexture = value;
}

inline ColorRgba *
Intersection::getObjectColor() const
{
    return objectColor;
}

inline void
Intersection::setObjectColor(ColorRgba *value)
{
    objectColor = value;
}

inline bool
Intersection::getNoShadowFlag() const
{
    return noShadowFlag;
}

inline void
Intersection::setNoShadowFlag(bool value)
{
    noShadowFlag = value;
}

template <>
inline bool
java::PriorityQueue<Intersection>::lessThan(
    const Intersection& a, const Intersection& b) const
{
    return a.getT() < b.getT();
}

#endif
