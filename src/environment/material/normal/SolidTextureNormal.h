#ifndef __SOLID_TEXTURE_NORMAL__
#define __SOLID_TEXTURE_NORMAL__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageTexture.h"
#include "vsdk/toolkit/media/solidTexture/procedural/BumpTextureFixture.h"

// One concrete subclass per POV-Ray bump/normal pattern (waves, ripples, bump_map, ...).
// `point` is already expressed in the owning material's texture space; the caller
// (BumpNormalShader) is responsible for applying that material's own
// textureTransformationInverse before calling applyTo.
class SolidTextureNormal {
  public:
    virtual ~SolidTextureNormal() = default;

    virtual void applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
        const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const = 0;
    virtual SolidTextureNormal *copy() const = 0;
};

#endif
