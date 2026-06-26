#ifndef __MARBLE_PIGMENT__
#define __MARBLE_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class MarblePigment : public SolidTexturePigment {
  public:
    MarblePigment(double turbulence, int octaves, const RGBAColorPalette *colorMap);
    ~MarblePigment() override;

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
