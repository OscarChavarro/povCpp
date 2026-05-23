/****************************************************************************
 *                     txttest.c
 *
 *  This module implements "fill-in-the-blank" pre-programmed texture
 *  functions for easy modification and testing. Create new textures here.
 *
 *****************************************************************************/
/*
    Some texture ideas garnered from SIGGRAPH '85 Volume 19 Number 3,
    "An Image Synthesizer" By Ken Perlin.
    Further Ideas Garnered from "The RenderMan Companion" (Addison Wesley)
*/

#include "media/TextureFixture.h"
#include "common/logger/Logger.h"
#include "common/LegacyBoolean.h"
#include <cstdio>
#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

/* Test new textures in the routines that follow */

/* The painted routines take an x,y,z point on an object and a pointer to the*/
/* object's texture description and return the color at that point */
/* Similar routines are granite, agate, marble. See txtcolor.c for examples. */

void
TextureFixture::painted1(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{

    /* Swirled()  */

    Vector3Dd colourVector;
    Vector3Dd result;
    int i;
    double scale = 1.0;
    double temp;
    RGBAColor newColour;

    if (debugEnabled) {
        Logger::info("painted1 %g %g %g\n", x, y, z);
    }

    result.x = 0.0;
    result.y = 0.0;
    result.z = 0.0;

    for (i = 0; i < 10; scale *= 2.0, i++) {
        TextureUtils::DNoise(&colourVector, x, y, z);
        temp = TextureUtils::Noise(colourVector.x * 4 * scale,
            colourVector.y * 4 * scale, colourVector.z * 4 * scale);
        temp = fabsInline(temp);
        result.x += temp / scale;
        result.y += temp / scale;
        result.z += temp / scale;
    }

    temp = result.x;
    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, temp);
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
TextureFixture::painted2(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    int brkindx;
    double turb;
    Vector3Dd textureTurbulence;
    RGBAColor colour1;
    RGBAColor colour2;

    /* You could change the parser to take two colors after PAINTED2, */
    /* but since the colormap is already parsed it's easier to use it during */
    /* testing. If the texture works out right you can change the parser later.
     */
    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&colour1, texture->Colour_Map, 0.1);
        TextureUtils::computeColour(&colour2, texture->Colour_Map, 0.9);
    } else {
        Color::makeColor(&colour1, 1.0, 1.0, 1.0);
        colour1.Alpha = 0.0;
        Color::makeColor(&colour2, 0.0, 1.0, 0.0);
        colour2.Alpha = 0.0;
    }

    if ((turb = texture->Turbulence) != 0.0) {
        TextureUtils::DTurbulence(
            &textureTurbulence, x, y, z, texture->Octaves);
        x += textureTurbulence.x * turb;
        y += textureTurbulence.y * turb;
        z += textureTurbulence.z * turb;
    }

    brkindx = (int)floorInline(x) + (int)floorInline(z);

    if (debugEnabled) {
        Logger::info("checker %g %g %g\n", x, y, z);
    }

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

    ;
}

void
TextureFixture::painted3(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    /* YOUR NAME HERE */
    ;
}

/* The bumpy routines take a point on an object,  a pointer to the */
/* object's texture description and the surface normal at that point and     */
/* return a peturb surface normal to create the illusion that the surface    */
/* has been displaced. */
/* Similar routines are ripples, dents, bumps. See txtbump.c for examples.  */
void
TextureFixture::bumpy1(
    double x, double y, double z, Texture *texture, Vector3Dd *normal, int debugEnabled)
{
}

/* Dan Farmer */
/* Same as bumpy1 except use VAdd for both cases of brkindex */
void
TextureFixture::bumpy2(
    double x, double y, double z, Texture *texture, Vector3Dd *normal, int debugEnabled)
{
}

/* Dan Farmer */
/* Same as bumpy2 except scale AFTER setting brkindex */
void
TextureFixture::bumpy3(
    double x, double y, double z, Texture *texture, Vector3Dd *normal, int debugEnabled)
{
}
