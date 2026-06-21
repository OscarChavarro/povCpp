#ifndef __MATERIAL__
#define __MATERIAL__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Material {
  public:
    virtual ~Material() {}

    virtual Material *copy() = 0;
    virtual Material *translate(Vector3Dd *vector) = 0;
    virtual Material *rotate(Vector3Dd *vector) = 0;
    virtual Material *scale(Vector3Dd *vector) = 0;
    virtual double getObjectIndexOfRefraction() const = 0;
    virtual Material *prependMaterialLayers(Material *existingMaterial) = 0;
    virtual void releaseFromOwner() = 0;
};

#endif
