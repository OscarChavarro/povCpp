#ifndef __SPOTTED_PIGMENT__
#define __SPOTTED_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class SpottedPigment : public SolidTexturePigment {
  public:
    explicit SpottedPigment(const RGBAColorPalette *colorMap);

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    const RGBAColorPalette *getColorMap() const;

  private:
    const RGBAColorPalette *colorMap;
};

#endif
