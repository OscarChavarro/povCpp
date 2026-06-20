#ifndef __CHECKER_TEXTURE_PIGMENT__
#define __CHECKER_TEXTURE_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class PovRayMaterial;

class CheckerTexturePigment : public SolidTexturePigment {
  public:
    CheckerTexturePigment(PovRayMaterial *texture1, PovRayMaterial *texture2);

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    void rotate(Vector3Dd *vector) override;
    void scale(Vector3Dd *vector) override;
    void translate(Vector3Dd *vector) override;

    PovRayMaterial *getTexture1() const;
    PovRayMaterial *getTexture2() const;

  private:
    PovRayMaterial *texture1;
    PovRayMaterial *texture2;
};

#endif
