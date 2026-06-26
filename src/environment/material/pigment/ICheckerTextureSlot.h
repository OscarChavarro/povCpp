#ifndef __I_CHECKER_TEXTURE_SLOT__
#define __I_CHECKER_TEXTURE_SLOT__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Material;
class SolidTexturePigment;

class ICheckerTextureSlot {
  public:
    virtual ~ICheckerTextureSlot() {}
    virtual Matrix4x4d *getTextureTransformationInverse() const = 0;
    virtual SolidTexturePigment *getPigment() const = 0;
    virtual ICheckerTextureSlot *copySlot() const = 0;
    virtual Material *rotate(Vector3Dd *vector) = 0;
    virtual Material *scale(Vector3Dd *vector) = 0;
    virtual Material *translate(Vector3Dd *vector) = 0;
};

#endif
