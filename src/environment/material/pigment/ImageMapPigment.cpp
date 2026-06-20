#include "environment/material/pigment/ImageMapPigment.h"

ImageMapPigment::ImageMapPigment(const ControlledRGBAImageHDRUncompressed *image) :
    image(image)
{
}

void
ImageMapPigment::colorAt(const Vector3Dd *point, ColorRgba *color, double smallTolerance,
    const ColorTextureFixture &colorFixture, const ImageTexture &mapFixture) const
{
    double x = point->x();
    double y = point->y();
    double z = point->z();
    mapFixture.imageMap(x, y, z, image, color, smallTolerance);
}

SolidTexturePigment *
ImageMapPigment::copy() const
{
    return new ImageMapPigment(image);
}

const ControlledRGBAImageHDRUncompressed *
ImageMapPigment::getImage() const
{
    return image;
}
