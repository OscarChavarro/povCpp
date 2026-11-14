#ifndef __RAY_SLAB_RECIPROCALS__
#define __RAY_SLAB_RECIPROCALS__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

/**
Reciprocals of a ray direction used by the slab test against axis aligned
boxes (Kay and Kajiya 1986). A component with magnitude under `1e-12` is
degenerate: the ray is parallel to that slab, its reciprocal is stored as zero
and the slab test checks the origin coordinate instead. It is computed once
per ray direction on the hottest path of a render, so its constructors are
inline in this header.
*/
class RaySlabReciprocals {
  private:
    double inverseX;
    double inverseY;
    double inverseZ;
    bool degenerateX;
    bool degenerateY;
    bool degenerateZ;

  public:
    RaySlabReciprocals();
    explicit RaySlabReciprocals(const Vector3Dd &direction);

    double getInverseX() const;
    double getInverseY() const;
    double getInverseZ() const;
    bool isDegenerateX() const;
    bool isDegenerateY() const;
    bool isDegenerateZ() const;
};

inline RaySlabReciprocals::RaySlabReciprocals() :
    inverseX(0.0), inverseY(0.0), inverseZ(0.0),
    degenerateX(true), degenerateY(true), degenerateZ(true)
{
}

inline RaySlabReciprocals::RaySlabReciprocals(const Vector3Dd &direction)
{
    degenerateX = direction.x() > -1e-12 && direction.x() < 1e-12;
    degenerateY = direction.y() > -1e-12 && direction.y() < 1e-12;
    degenerateZ = direction.z() > -1e-12 && direction.z() < 1e-12;
    inverseX = degenerateX ? 0.0 : 1.0 / direction.x();
    inverseY = degenerateY ? 0.0 : 1.0 / direction.y();
    inverseZ = degenerateZ ? 0.0 : 1.0 / direction.z();
}

inline double RaySlabReciprocals::getInverseX() const { return inverseX; }
inline double RaySlabReciprocals::getInverseY() const { return inverseY; }
inline double RaySlabReciprocals::getInverseZ() const { return inverseZ; }
inline bool RaySlabReciprocals::isDegenerateX() const { return degenerateX; }
inline bool RaySlabReciprocals::isDegenerateY() const { return degenerateY; }
inline bool RaySlabReciprocals::isDegenerateZ() const { return degenerateZ; }

#endif
