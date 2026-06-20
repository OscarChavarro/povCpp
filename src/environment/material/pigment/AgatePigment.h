#ifndef __AGATE_PIGMENT__
#define __AGATE_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class AgatePigment : public SolidTexturePigment {
  public:
    AgatePigment(int octaves, const RGBAColorPalette *colorMap);

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    int getOctaves() const;
    const RGBAColorPalette *getColorMap() const;

  private:
    int octaves;
    const RGBAColorPalette *colorMap;
};

#endif
