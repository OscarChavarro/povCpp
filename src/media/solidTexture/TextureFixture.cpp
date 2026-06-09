/**
Fill-in-the-blank pre-programmed texture functions for easy modification and testing.
Create new experimental textures here before promoting them to colorTextureFixture.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
"The RenderMan Companion" (Addison Wesley).
*/

#include "media/solidTexture/TextureFixture.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include <cstdio>
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"

/**
Painted1: takes an x,y,z point on an object and returns the color at that point.
See colorTextureFixture for similar finished textures (granite, agate, marble, etc.).
*/
void
textureFixture::painted1(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{

    // Swirled()
    Vector3Dd colourVector;
    Vector3Dd result;
    int i;
    double scale = 1.0;
    double temp;
    RGBAColor newColour;


    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;

    for (i = 0; i < 10; scale *= 2.0, i++) {
        textureUtils::instance().DNoise(&colourVector, x, y, z);
        temp = textureUtils::instance().Noise(colourVector.x() * 4 * scale,
            colourVector.y() * 4 * scale, colourVector.z() * 4 * scale);
        temp = textureUtils::instance().fabsInline(temp);
        rx += temp / scale;
        ry += temp / scale;
        rz += temp / scale;
    }
    result = Vector3Dd(rx, ry, rz);

    temp = result.x();
    if (texture->colorMap != nullptr) {
        textureUtils::instance().computeColour(&newColour, texture->colorMap, temp);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += temp;
    colour->Green += temp;
    colour->Blue += temp;
}

void
textureFixture::painted2(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    int brkindx;
    double turb;
    Vector3Dd textureTurbulence;
    RGBAColor colour1;
    RGBAColor colour2;

    // You could change the parser to take two colors after PAINTED2, but since
    // the colormap is already parsed it's easier to use it during testing.
    // If the texture works out right you can change the parser later.
    if (texture->colorMap != nullptr) {
        textureUtils::instance().computeColour(&colour1, texture->colorMap, 0.1);
        textureUtils::instance().computeColour(&colour2, texture->colorMap, 0.9);
    } else {
        Color::makeColor(&colour1, 1.0, 1.0, 1.0);
        colour1.Alpha = 0.0;
        Color::makeColor(&colour2, 0.0, 1.0, 0.0);
        colour2.Alpha = 0.0;
    }

    if ((turb = texture->turbulence) != 0.0) {
        textureUtils::instance().DTurbulence(
            &textureTurbulence, x, y, z, texture->octaves);
        x += textureTurbulence.x() * turb;
        y += textureTurbulence.y() * turb;
        z += textureTurbulence.z() * turb;
    }

    brkindx = (int)textureUtils::instance().floorInline(x) + (int)textureUtils::instance().floorInline(z);


    if (brkindx & 1) {
        colour->Red = colour1.Red;
        colour->Green = colour1.Green;
        colour->Blue = colour1.Blue;
        colour->Alpha = colour1.Alpha;
    } else {
        colour->Red = colour2.Red;
        colour->Green = colour2.Green;
        colour->Blue = colour2.Blue;
        colour->Alpha = colour2.Alpha;
    }
    return;
}

void
textureFixture::painted3(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    ;
}

/**
Bumpy1: takes a point, texture, and surface normal; returns a perturbed normal.
See bumpTextureFixture for similar finished bump textures (ripples, dents, bumps, etc.).
*/
void
textureFixture::bumpy1(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
}

/** Same as bumpy1 except use VAdd for both cases of brkindex. */
void
textureFixture::bumpy2(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
}

/** Same as bumpy2 except scale AFTER setting brkindex. */
void
textureFixture::bumpy3(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
}
