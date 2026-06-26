#ifndef __GRANITE_PIGMENT__
#define __GRANITE_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class GranitePigment : public SolidTexturePigment {
  public:
    explicit GranitePigment(const RGBAColorPalette *colorMap);
    ~GranitePigment() override;

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;

  private:
    const RGBAColorPalette *colorMap;
};

#endif
