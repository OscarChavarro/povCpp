#ifndef __BRICK_PIGMENT__
#define __BRICK_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class BrickPigment : public SolidTexturePigment {
  public:
    BrickPigment(ColorRgba *color1, ColorRgba *color2, double mortar);

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    ColorRgba *getColor1() const;
    ColorRgba *getColor2() const;
    double getMortar() const;

  private:
    ColorRgba *color1;
    ColorRgba *color2;
    double mortar;
};

#endif
