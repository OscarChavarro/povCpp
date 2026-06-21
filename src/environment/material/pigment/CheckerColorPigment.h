#ifndef __CHECKER_COLOR_PIGMENT__
#define __CHECKER_COLOR_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class CheckerColorPigment : public SolidTexturePigment {
  public:
    CheckerColorPigment(ColorRgba *color1, ColorRgba *color2);
    ~CheckerColorPigment() override;

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    ColorRgba *getColor1() const;
    ColorRgba *getColor2() const;

  private:
    ColorRgba *color1;
    ColorRgba *color2;
};

#endif
