#ifndef __LEOPARD_PIGMENT__
#define __LEOPARD_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class LeopardPigment : public SolidTexturePigment {
  public:
    LeopardPigment(double turbulence, int octaves, const RGBAColorPalette *colorMap);
    ~LeopardPigment() override;

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
