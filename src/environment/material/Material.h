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

    // Disposes of this material on behalf of an owning object (its objectTexture)
    // being destroyed. A privately-owned material is freed; a shared/aliased one
    // (the scene's default texture, or a #declare'd constant texture) is not freed
    // here, only its alias bookkeeping is closed out, because it is owned
    // elsewhere. After this call the pointer must not be used again.
    virtual void releaseFromOwner() = 0;
};

#endif
