#ifndef __UNSUPPORTED_BUMP_NORMAL__
#define __UNSUPPORTED_BUMP_NORMAL__

#include "environment/material/normal/SolidTextureNormal.h"

// Stand-in for BUMPY1/BUMPY2/BUMPY3: parsed by TextureParser.cpp but, in the original
// BumpNormalShader switch, fell into `default: break` and never wrote to newNormal (not
// even a fallback to surfaceNormal). applyTo() preserves that exact silence.
class UnsupportedBumpNormal : public SolidTextureNormal {
  public:
    void applyTo(const Vector3Dd *point, Vector3Dd *newNormal,
        const BumpTextureFixture &bumpFixture, const ImageTexture &mapFixture) const override;
    SolidTextureNormal *copy() const override;
};

#endif
