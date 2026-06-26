#ifndef __ONION_PIGMENT__
#define __ONION_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class OnionPigment : public SolidTexturePigment {
  public:
    OnionPigment(double turbulence, int octaves, const RGBAColorPalette *colorMap);
    ~OnionPigment() override;

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    int getOctaves() const;

  private:
    double turbulence;
    int octaves;
    const RGBAColorPalette *colorMap;
};

#endif
