/****************************************************************************
 *                     txtcolor.c
 *
 *  This module implements solid texturing functions that modify the color
 *  transparency of an object's surface.
 *
References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
 *****************************************************************************/

/*
    Some texture ideas garnered from SIGGRAPH '85 Volume 19 Number 3,
    "An Image Synthesizer" By Ken Perlin.
    Further Ideas Garnered from "The RenderMan Companion" (Addison Wesley).
*/

#include "media/ColorTextureFixture.h"
#include "common/logger/Logger.h"
#include <cstdio>
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/MapTextureFixture.h"
#include "media/Texture.h"
#include "media/TextureFixture.h"

static constexpr double COORDINATE_LIMIT = 1.0e17;

void
ColorTextureFixture::colourAt(
    RGBAColor *colour, Texture *texture, Vector3Dd *intersectionPoint,
    int debugEnabled, double smallTolerance)
{
    double x;
    double y;
    double z;
    Vector3Dd transformedPoint;

    if ((intersectionPoint->x() > COORDINATE_LIMIT) ||
        (intersectionPoint->y() > COORDINATE_LIMIT) ||
        (intersectionPoint->z() > COORDINATE_LIMIT) ||
        (intersectionPoint->x() < -COORDINATE_LIMIT) ||
        (intersectionPoint->y() < -COORDINATE_LIMIT) ||
        (intersectionPoint->z() < -COORDINATE_LIMIT)) {
        *&transformedPoint = Vector3Dd(0.0, 0.0, 0.0);
    } else {
        if (texture->textureTransformation) {
            transformedPoint =
                texture->textureTransformationInverse->transpose().multiply(
                    *intersectionPoint);
        } else {
            transformedPoint = *intersectionPoint;
        }
    }

    x = transformedPoint.x();
    y = transformedPoint.y();
    z = transformedPoint.z();

    switch (texture->textureNumber) {
    case Texture::NO_TEXTURE:
        /* No colouring texture has been specified - make it black. */
        Color::makeColor(colour, 0.0, 0.0, 0.0);
        colour->Alpha = 0.0;
        break;

    case Texture::COLOUR_TEXTURE:
        colour->Red += texture->Colour1->Red;
        colour->Green += texture->Colour1->Green;
        colour->Blue += texture->Colour1->Blue;
        colour->Alpha += texture->Colour1->Alpha;
        break;

    case Texture::BOZO_TEXTURE:
        ColorTextureFixture::bozo(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::MARBLE_TEXTURE:
        ColorTextureFixture::marble(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::WOOD_TEXTURE:
        ColorTextureFixture::wood(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::BRICK_TEXTURE:
        ColorTextureFixture::brick(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::CHECKER_TEXTURE:
        ColorTextureFixture::checker(x, y, z, texture, colour, debugEnabled, smallTolerance);
        break;

    case Texture::CHECKER_TEXTURE_TEXTURE:
        ColorTextureFixture::checkerTexture(x, y, z, texture, colour, debugEnabled, smallTolerance);
        break;

    case Texture::SPOTTED_TEXTURE:
        ColorTextureFixture::spotted(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::AGATE_TEXTURE:
        ColorTextureFixture::agate(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::GRANITE_TEXTURE:
        ColorTextureFixture::granite(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::GRADIENT_TEXTURE:
        ColorTextureFixture::gradient(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::IMAGEMAP_TEXTURE:
        MapTextureFixture::imageMap(
            x, y, z, texture, colour, debugEnabled, smallTolerance);
        break;

    case Texture::ONION_TEXTURE:
        ColorTextureFixture::onion(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::LEOPARD_TEXTURE:
        ColorTextureFixture::leopard(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::PAINTED1_TEXTURE:
        TextureFixture::painted1(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::PAINTED2_TEXTURE:
        TextureFixture::painted2(x, y, z, texture, colour, debugEnabled);
        break;

    case Texture::PAINTED3_TEXTURE:
        TextureFixture::painted3(x, y, z, texture, colour, debugEnabled);
        break;
    }
}

// [PERL1985].290 - Agate: combines turbulence modulation with periodic wave function
void
ColorTextureFixture::agate(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    double noise;
    double hue;
    RGBAColor newColour;

    noise = TextureUtils::cycloidal(
                1.3 * TextureUtils::Turbulence(x, y, z, texture->Octaves) +
                1.1 * z) +
            1;
    noise *= 0.5;
    noise = pow(noise, 0.77);

    if (debugEnabled) {
        Logger::info("agate %g %g %g noise %g\n", x, y, z, noise);
    }

    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    hue = 1.0 - noise;

    if (noise < 0.5) {
        colour->Red += (1.0 - (noise / 10));
        colour->Green += (1.0 - (noise / 5));
        colour->Blue += hue;
    } else if (noise < 0.6) {
        colour->Red += 0.9;
        colour->Green += 0.7;
        colour->Blue += hue;
    } else {
        colour->Red += (0.6 + hue);
        colour->Green += (0.3 + hue);
        colour->Blue += hue;
    }
}

// [PERL1985].290 - Bozo: displaced Noise() via DTurbulence gradient perturbation
void
ColorTextureFixture::bozo(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    double noise;
    double turb;
    RGBAColor newColour;
    Vector3Dd bozoTurbulence;

    if (debugEnabled) {
        Logger::info("bozo %g %g %g ", x, y, z);
    }

    if ((turb = texture->Turbulence) != 0.0) {
        TextureUtils::DTurbulence(&bozoTurbulence, x, y, z, texture->Octaves);
        x += bozoTurbulence.x() * turb;
        y += bozoTurbulence.y() * turb;
        z += bozoTurbulence.z() * turb;
    }

    noise = TextureUtils::Noise(x, y, z);

    if (debugEnabled) {
        Logger::info("noise %g\n", noise);
    }

    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    if (noise < 0.4) {
        colour->Red += 1.0;
        colour->Green += 1.0;
        colour->Blue += 1.0;
        return;
    }

    if (noise < 0.6) {
        colour->Green += 1.0;
        return;
    }

    if (noise < 0.8) {
        colour->Blue += 1.0;
        return;
    }

    colour->Red += 1.0;
}

void
ColorTextureFixture::brick(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    double xr;
    double yr;
    double zr;

    xr = fabs(fmod(x, 1.0));
    yr = fabs(fmod(y, 1.0));
    zr = fabs(fmod(z, 1.0));

    *colour = *texture->Colour2;

    if (debugEnabled) {
        Logger::info("brick %g %g %g\n", x, y, z);
    }

    if (xr > 0 && xr < texture->Mortar) {
        *colour = *texture->Colour1;
        return;
    }
    if (yr > 0 && yr < texture->Mortar) {
        *colour = *texture->Colour1;
        return;
    }
    if (zr > 0 && zr < texture->Mortar) {
        *colour = *texture->Colour1;
    }
}

void
ColorTextureFixture::checker(double x, double y, double z, Texture *texture,
    RGBAColor *colour, int debugEnabled, double smallTolerance)
{
    int brkindx;

    x += smallTolerance; /* add a small offset to x, y, z, axes to prevent
                             noise */
    y += smallTolerance;
    z += smallTolerance;

    /* AAC: was just x + z */
    /* AAC: GeometryConstants::Small_Tolerance added to get around Microsoft C (int) bug */
    brkindx = (int)(TextureUtils::floorInline(x) + TextureUtils::floorInline(y) + TextureUtils::floorInline(z));

    /* INSTRUMENTATION: Print texture parameters on first call */
    static int checkerDiagCallCount = 0;
    if (checkerDiagCallCount++ == 0) {
        Logger::info("[CHECKER-DIAG] textureNumber=%d reflection=%g\n",
            texture->textureNumber, texture->objectReflection);
        if (texture->Colour1) {
            Logger::info("[CHECKER-DIAG] Colour1 R=%g G=%g B=%g A=%g\n",
                texture->Colour1->Red, texture->Colour1->Green,
                texture->Colour1->Blue, texture->Colour1->Alpha);
        } else {
            Logger::info("[CHECKER-DIAG] Colour1=NULL\n");
        }
        if (texture->Colour2) {
            Logger::info("[CHECKER-DIAG] Colour2 R=%g G=%g B=%g A=%g\n",
                texture->Colour2->Red, texture->Colour2->Green,
                texture->Colour2->Blue, texture->Colour2->Alpha);
        } else {
            Logger::info("[CHECKER-DIAG] Colour2=NULL\n");
        }
        if (texture->textureTransformation) {
            for (int r = 0; r < 4; r++) {
                Logger::info("[CHECKER-DIAG] Transform row%d: %g %g %g %g\n", r,
                    texture->textureTransformation->get(r, 0),
                    texture->textureTransformation->get(r, 1),
                    texture->textureTransformation->get(r, 2),
                    texture->textureTransformation->get(r, 3));
            }
        } else {
            Logger::info("[CHECKER-DIAG] textureTransformation=NULL\n");
        }
    }

    if (debugEnabled) {
        Logger::info("checker %g %g %g\n", x, y, z);
    }

    if (brkindx & 1) {
        colour->Red += texture->Colour1->Red;
        colour->Green += texture->Colour1->Green;
        colour->Blue += texture->Colour1->Blue;
        colour->Alpha += texture->Colour1->Alpha;
    } else {
        colour->Red += texture->Colour2->Red;
        colour->Green += texture->Colour2->Green;
        colour->Blue += texture->Colour2->Blue;
        colour->Alpha += texture->Colour2->Alpha;
    }
}

void
ColorTextureFixture::checkerTexture(double x, double y, double z,
    Texture *texture, RGBAColor *colour, int debugEnabled,
    double smallTolerance)
{
    int brkindx;
    Vector3Dd point;

    x += smallTolerance; /* add a small offset to x, y, z, axes to prevent
                             noise */
    y += smallTolerance;
    z += smallTolerance;

    brkindx = (int)(TextureUtils::floorInline(x) + TextureUtils::floorInline(y) + TextureUtils::floorInline(z));

    if (debugEnabled) {
        Logger::info("checker_texture %g %g %g\n", x, y, z);
    }

    *&point = Vector3Dd(x, y, z);

    if (brkindx & 1) {
        ColorTextureFixture::colourAt(
            colour, ((Texture *)texture->Colour1), &point, debugEnabled,
            smallTolerance);
    } else {
        ColorTextureFixture::colourAt(
            colour, ((Texture *)texture->Colour2), &point, debugEnabled,
            smallTolerance);
    }
}

/*
    Color Gradient Texture - gradient based on the fractional values of x, y or
    z, based on whether or not the given directional vector is a 1.0 or a 0.0.
    Note - ONLY works with colour maps, preferably one that is circular - i.e.
    the last defined colour (value 1.001) is the same as the first colour (with
    a value of 0.0) in the map.  The basic concept of this is from DBW Render,
    but Dave Wecker's only supports simple Y axis gradients.
*/
void
ColorTextureFixture::gradient(
    double x, double y, double z, Texture *texture, RGBAColor *colour,
    int debugEnabled)
{
    RGBAColor newColour;
    double value = 0.0;
    double turb;
    Vector3Dd gradTurbulence;

    if ((turb = texture->Turbulence) != 0.0) {
        TextureUtils::DTurbulence(&gradTurbulence, x, y, z, texture->Octaves);
        x += gradTurbulence.x() * turb;
        y += gradTurbulence.y() * turb;
        z += gradTurbulence.z() * turb;
    }

    if (texture->Colour_Map == nullptr) {
        return;
    }
    if (texture->textureGradient.x() != 0.0) {
        x = TextureUtils::fabsInline(x);
        value += x - TextureUtils::floorInline(x); /* obtain fractional X component */
    }
    if (texture->textureGradient.y() != 0.0) {
        y = TextureUtils::fabsInline(y);
        value += y - TextureUtils::floorInline(y); /* obtain fractional Y component */
    }
    if (texture->textureGradient.z() != 0.0) {
        z = TextureUtils::fabsInline(z);
        value += z - TextureUtils::floorInline(z); /* obtain fractional Z component */
    }
    value = ((value > 1.0) ? fmod(value, 1.0) : value); /* clamp to 1.0 */

    if (debugEnabled) {
        Logger::info("gradient %g %g %g value %g\n", x, y, z, value);
    }

    TextureUtils::computeColour(&newColour, texture->Colour_Map, value);
    colour->Red += newColour.Red;
    colour->Green += newColour.Green;
    colour->Blue += newColour.Blue;
    colour->Alpha += newColour.Alpha;
}

/*
    Granite - kind of a union of the "spotted" and the "dented" textures,
    using a 1/f fractal noise function for color values.  Typically used
    w/ small scaling values.  Should work with colour maps for pink granite...
    [PERL1985].290 - Granite: 1/f fractal composition of Noise() over octaves
*/
void
ColorTextureFixture::granite(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    int i;
    double temp;
    double noise = 0.0;
    double freq = 1.0;
    RGBAColor newColour;

    for (i = 0; i < 6; freq *= 2.0, i++) {
        temp =
            0.5 - TextureUtils::Noise(x * 4 * freq, y * 4 * freq, z * 4 * freq);
        temp = TextureUtils::fabsInline(temp);
        noise += temp / freq;
    }

    if (debugEnabled) {
        Logger::info("granite %g %g %g noise %g\n", x, y, z, noise);
    }

    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += noise; /* "white (1.0) * noise" */
    colour->Green += noise;
    colour->Blue += noise;
}

// [PERL1985].291 - Marble: sine-wave color splining with turbulence-perturbed phase
// Implements: x = point[1] + turbulence(point); color = marble_color(sin(x))
void
ColorTextureFixture::marble(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    double noise;
    double hue;
    RGBAColor newColour;

    noise = TextureUtils::triangleWave(
        x + TextureUtils::Turbulence(x, y, z, texture->Octaves) *
                texture->Turbulence);

    if (debugEnabled) {
        Logger::info("marble %g %g %g noise %g \n", x, y, z, noise);
    }

    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    if (noise < 0.0) {
        colour->Red += 0.9;
        colour->Green += 0.8;
        colour->Blue += 0.8;
    } else if (noise < 0.9) {
        colour->Red += 0.9;
        hue = 0.8 - noise * 0.8;
        colour->Green += hue;
        colour->Blue += hue;
    }
}

/*
    With a little reflectivity and brilliance, can look like organ pipe
    metal.    With tiny scaling values can look like masonry or concrete.
    Works with color maps.
*/
// [PERL1985].290 - Spotted: basic noise-based random surface texture (Spotted Donut example)
// Implements: color = white * Noise(point)
void
ColorTextureFixture::spotted(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    double noise;
    RGBAColor newColour;

    noise = TextureUtils::Noise(x, y, z);

    if (debugEnabled) {
        Logger::info("spotted %g %g %g\n", x, y, z);
    }

    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += noise; /* "white (1.0) * noise" */
    colour->Green += noise;
    colour->Blue += noise;
}

// [PERL1985].291 - Wood: turbulence-based ring patterns via periodic wave on radial distance
// Uses DTurbulence gradient perturbation + cycloidal() for ring bands
void
ColorTextureFixture::wood(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    double noise;
    double length;
    Vector3Dd woodTurbulence;
    Vector3Dd point;
    RGBAColor newColour;

    TextureUtils::DTurbulence(&woodTurbulence, x, y, z, texture->Octaves);

    if (debugEnabled) {
        Logger::info("wood %g %g %g", x, y, z);
    }

    double pointX =
        TextureUtils::cycloidal((x + woodTurbulence.x()) * texture->Turbulence);
    double pointY =
        TextureUtils::cycloidal((y + woodTurbulence.y()) * texture->Turbulence);

    pointX += x;
    pointY += y;

    /*  point.z += z;         Deleted per David Buck --  BP 7/91 */

    point = Vector3Dd(pointX, pointY, 0.0);
    length = point.length();

    noise = TextureUtils::triangleWave(length);

    if (debugEnabled) {
        Logger::info("noise %g\n", noise);
    }

    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    if (noise > 0.6) {
        colour->Red += 0.4;
        colour->Green += 0.133;
        colour->Blue += 0.066;
    } else {
        colour->Red += 0.666;
        colour->Green += 0.312;
        colour->Blue += 0.2;
    }
}

/* Two new textures by Scott Taylor LEOPARD & ONION */
/* SWT 7/18/91 */
void
ColorTextureFixture::leopard(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    /* The variable noise is not used as noise in this function */
    double noise;
    double turb;
    double temp1;
    double temp2;
    double temp3;
    RGBAColor newColour;
    Vector3Dd leopardTurbulence;

    if (debugEnabled) {
        Logger::info("leopard %g %g %g ", x, y, z);
    }

    if ((turb = texture->Turbulence) != 0.0) {
        TextureUtils::DTurbulence(
            &leopardTurbulence, x, y, z, texture->Octaves);
        x += leopardTurbulence.x() * turb;
        y += leopardTurbulence.y() * turb;
        z += leopardTurbulence.z() * turb;
    }
    /* This form didn't work with Zortech 386 compiler */
    /* noise = (((sin(x)+sin(y)+sin(z))/3)*((sin(x)+sin(y)+sin(z))/3)); */
    /* So we break it down. */
    temp1 = sin(x);
    temp2 = sin(y);
    temp3 = sin(z);
    noise = (((temp1 + temp2 + temp3) / 3)*((temp1 + temp2 + temp3) / 3));

    if (debugEnabled) {
        Logger::info("temp123 %g %g %g  ", temp1, temp2, temp3);
    }

    if (debugEnabled) {
        Logger::info("noise %g\n", noise);
    }

    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += noise;
    colour->Green += noise;
    colour->Blue += noise;
}

/* SWT 7/18/91 */
void
ColorTextureFixture::onion(
    double x, double y, double z, Texture *texture, RGBAColor *colour, int debugEnabled)
{
    /* The variable noise is not used as noise in this function */
    double noise;
    double turb;
    RGBAColor newColour;
    Vector3Dd onionTurbulence;

    if (debugEnabled) {
        Logger::info("onion %g %g %g ", x, y, z);
    }

    if ((turb = texture->Turbulence) != 0.0) {
        TextureUtils::DTurbulence(&onionTurbulence, x, y, z, texture->Octaves);
        x += onionTurbulence.x() * turb;
        y += onionTurbulence.y() * turb;
        z += onionTurbulence.z() * turb;
    }

    /* This ramp goes 0-1,1-0,0-1,1-0...
    noise =
    (fmod(std::sqrt(((x)*(x))+((y)*(y))+((z)*(z))),2.0)-1.0);
    if (noise<0.0) {noise = 0.0-noise;}
    */

    /* This ramp goes 0-1,0-1,0-1,0-1... */
    noise = (fmod(
        std::sqrt(((x)*(x)) + ((y)*(y)) + ((z)*(z))),
        1.0));

    if (debugEnabled) {
        Logger::info("noise %g\n", noise);
    }

    if (texture->Colour_Map != nullptr) {
        TextureUtils::computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += noise;
    colour->Green += noise;
    colour->Blue += noise;
}
