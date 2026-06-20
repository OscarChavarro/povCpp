#ifndef __COLOUR_PIGMENT__
#define __COLOUR_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class ColourPigment : public SolidTexturePigment {
  public:
    explicit ColourPigment(ColorRgba *color1);

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    ColorRgba *getColor1() const;
    bool needsTransform() const override;

  private:
    ColorRgba *color1;
};

#endif
