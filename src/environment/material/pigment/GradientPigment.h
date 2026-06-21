#ifndef __GRADIENT_PIGMENT__
#define __GRADIENT_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class GradientPigment : public SolidTexturePigment {
  public:
    GradientPigment(double turbulence, int octaves, const RGBAColorPalette *colorMap, const Vector3Dd &textureGradient);
    ~GradientPigment() override;

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    double getTurbulence() const;
    int getOctaves() const;
    const RGBAColorPalette *getColorMap() const;
    const Vector3Dd &getTextureGradient() const;

  private:
    double turbulence;
    int octaves;
    const RGBAColorPalette *colorMap;
    Vector3Dd textureGradient;
};

#endif
