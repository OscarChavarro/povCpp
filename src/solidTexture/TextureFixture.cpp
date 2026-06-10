/**
Fill-in-the-blank pre-programmed texture functions for easy modification and testing.
Create new experimental textures here before promoting them to ColorTextureFixture.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
"The RenderMan Companion" (Addison Wesley).
*/

#include <cstdio>
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/Texture.h"
#include "solidTexture/TextureFixture.h"

TextureFixture::TextureFixture(ProceduralNoise *proceduralNoise)
    : proceduralNoise(proceduralNoise)
{
}

/**
Painted1: takes an x,y,z point on an object and returns the color at that point.
See ColorTextureFixture for similar finished textures (granite, agate, marble, etc.).
*/
void
TextureFixture::painted1(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{

    // Swirled()
    Vector3Dd colorVector;
    Vector3Dd result;
    int i;
    double scale = 1.0;
    double temp;
    ColorRgba newColor;


    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;

    for (i = 0; i < 10; scale *= 2.0, i++) {
        proceduralNoise->dNoise(&colorVector, x, y, z);
        temp = proceduralNoise->noise(colorVector.x() * 4 * scale,
            colorVector.y() * 4 * scale, colorVector.z() * 4 * scale);
        temp = TextureUtils::instance().fabsInline(temp);
        rx += temp / scale;
        ry += temp / scale;
        rz += temp / scale;
    }
    result = Vector3Dd(rx, ry, rz);

    temp = result.x();
    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, temp);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    color->setR(color->getR() + temp);
    color->setG(color->getG() + temp);
    color->setB(color->getB() + temp);
}

void
TextureFixture::painted2(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    int brkindx;
    double turb;
    Vector3Dd textureTurbulence;
    ColorRgba colour1;
    ColorRgba color2;

    // You could change the parser to take two colors after PAINTED2, but since
    // the colormap is already parsed it's easier to use it during testing.
    // If the texture works out right you can change the parser later.
    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColor(&colour1, texture->colorMap, 0.1);
        TextureUtils::instance().computeColor(&color2, texture->colorMap, 0.9);
    } else {
        colour1.setR(1.0); colour1.setG(1.0); colour1.setB(1.0); colour1.setA(0.0);
        color2.setR(0.0); color2.setG(1.0); color2.setB(0.0); color2.setA(0.0);
    }

    if ((turb = texture->turbulence) != 0.0) {
        proceduralNoise->dTurbulence(
            &textureTurbulence, x, y, z, texture->octaves);
        x += textureTurbulence.x() * turb;
        y += textureTurbulence.y() * turb;
        z += textureTurbulence.z() * turb;
    }

    brkindx = (int)TextureUtils::instance().floorInline(x) + (int)TextureUtils::instance().floorInline(z);


    if (brkindx & 1) {
        color->setR(colour1.getR());
        color->setG(colour1.getG());
        color->setB(colour1.getB());
        color->setA(colour1.getA());
    } else {
        color->setR(color2.getR());
        color->setG(color2.getG());
        color->setB(color2.getB());
        color->setA(color2.getA());
    }
    return;
}

void
TextureFixture::painted3(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    ;
}

/**
Bumpy1: takes a point, texture, and surface normal; returns a perturbed normal.
See BumpTextureFixture for similar finished bump textures (ripples, dents, bumps, etc.).
*/
void
TextureFixture::bumpy1(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
}

/** Same as bumpy1 except use VAdd for both cases of brkindex. */
void
TextureFixture::bumpy2(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
}

/** Same as bumpy2 except scale AFTER setting brkindex. */
void
TextureFixture::bumpy3(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
}
