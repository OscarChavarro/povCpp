#ifndef __IMAGE_MAP_PIGMENT__
#define __IMAGE_MAP_PIGMENT__

#include "environment/material/pigment/SolidTexturePigment.h"

class ImageMapPigment : public SolidTexturePigment {
  public:
    explicit ImageMapPigment(const ControlledRGBAImageHDRUncompressed *image);

    void colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
        const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const override;
    SolidTexturePigment *copy() const override;
    const ControlledRGBAImageHDRUncompressed *getImage() const;

  private:
    const ControlledRGBAImageHDRUncompressed *image;
};

#endif
