/**
Implements solid texturing functions that modify the color of an object's surface.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
"The RenderMan Companion" (Addison Wesley).
*/

#include <cstdio>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/ColorTextureFixture.h"
#include "media/solidTexture/MapTextureFixture.h"
#include "media/solidTexture/SolidTextureColorTextures.h"
#include "media/solidTexture/Texture.h"
#include "media/solidTexture/TextureFixture.h"

static constexpr double COORDINATE_LIMIT = 1.0e17;

void
ColorTextureFixture::colourAt(
    RGBAColor *colour, Texture *texture, Vector3Dd *intersectionPoint, double smallTolerance)
{
    double x;
    double y;
    double z;
    Vector3Dd transformedPoint;
    MapTextureFixture mapFixture;
    TextureFixture textureFixture;

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
    case (int)SolidTextureColorTextures::NO_TEXTURE:
        // No colouring texture has been specified - make it black
        Color::makeColor(colour, 0.0, 0.0, 0.0);
        colour->Alpha = 0.0;
        break;

    case (int)SolidTextureColorTextures::COLOUR_TEXTURE:
        colour->Red += texture->color1->Red;
        colour->Green += texture->color1->Green;
        colour->Blue += texture->color1->Blue;
        colour->Alpha += texture->color1->Alpha;
        break;

    case (int)SolidTextureColorTextures::BOZO_TEXTURE:
        bozo(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::MARBLE_TEXTURE:
        marble(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::WOOD_TEXTURE:
        wood(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::BRICK_TEXTURE:
        brick(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::CHECKER_TEXTURE:
        checker(x, y, z, texture, colour, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE:
        checkerTexture(x, y, z, texture, colour, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::SPOTTED_TEXTURE:
        spotted(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::AGATE_TEXTURE:
        agate(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::GRANITE_TEXTURE:
        granite(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::GRADIENT_TEXTURE:
        gradient(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::IMAGEMAP_TEXTURE:
        mapFixture.imageMap(x, y, z, texture, colour, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::ONION_TEXTURE:
        onion(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::LEOPARD_TEXTURE:
        leopard(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::PAINTED1_TEXTURE:
        textureFixture.painted1(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::PAINTED2_TEXTURE:
        textureFixture.painted2(x, y, z, texture, colour);
        break;

    case (int)SolidTextureColorTextures::PAINTED3_TEXTURE:
        textureFixture.painted3(x, y, z, texture, colour);
        break;
    }
}

/** [PERL1985].290 - Agate: combines turbulence modulation with periodic wave function. */
void
ColorTextureFixture::agate(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    double noise;
    double hue;
    RGBAColor newColour;

    noise = TextureUtils::instance().cycloidal(
                1.3 * TextureUtils::instance().Turbulence(x, y, z, texture->octaves) +
                1.1 * z) +
            1;
    noise *= 0.5;
    noise = pow(noise, 0.77);


    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColour(&newColour, texture->colorMap, noise);
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

/** [PERL1985].290 - Bozo: displaced Noise() via DTurbulence gradient perturbation. */
void
ColorTextureFixture::bozo(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    double noise;
    double turb;
    RGBAColor newColour;
    Vector3Dd bozoTurbulence;

    if ((turb = texture->turbulence) != 0.0) {
        TextureUtils::instance().DTurbulence(&bozoTurbulence, x, y, z, texture->octaves);
        x += bozoTurbulence.x() * turb;
        y += bozoTurbulence.y() * turb;
        z += bozoTurbulence.z() * turb;
    }

    noise = TextureUtils::instance().Noise(x, y, z);

    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColour(&newColour, texture->colorMap, noise);
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
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    double xr;
    double yr;
    double zr;

    xr = fabs(fmod(x, 1.0));
    yr = fabs(fmod(y, 1.0));
    zr = fabs(fmod(z, 1.0));
    *colour = *texture->color2;

    if (xr > 0 && xr < texture->mortar) {
        *colour = *texture->color1;
        return;
    }
    if (yr > 0 && yr < texture->mortar) {
        *colour = *texture->color1;
        return;
    }
    if (zr > 0 && zr < texture->mortar) {
        *colour = *texture->color1;
    }
}

void
ColorTextureFixture::checker(double x, double y, double z, Texture *texture,
    RGBAColor *colour, double smallTolerance)
{
    int brkindx;

    x += smallTolerance; // add a small offset to x, y, z, axes to preventd noise
    y += smallTolerance;
    z += smallTolerance;

    // AAC: was just x + z
    // AAC: GeometryConstants::Small_Tolerance added to get around Microsoft C (int) bug
    brkindx = (int)(TextureUtils::instance().floorInline(x) + TextureUtils::instance().floorInline(y) + TextureUtils::instance().floorInline(z));

    if (brkindx & 1) {
        colour->Red += texture->color1->Red;
        colour->Green += texture->color1->Green;
        colour->Blue += texture->color1->Blue;
        colour->Alpha += texture->color1->Alpha;
    } else {
        colour->Red += texture->color2->Red;
        colour->Green += texture->color2->Green;
        colour->Blue += texture->color2->Blue;
        colour->Alpha += texture->color2->Alpha;
    }
}

void
ColorTextureFixture::checkerTexture(double x, double y, double z,
    Texture *texture, RGBAColor *colour,
    double smallTolerance)
{
    int brkindx;
    Vector3Dd point;

    x += smallTolerance; // add a small offset to x, y, z, axes to prevent noise
    y += smallTolerance;
    z += smallTolerance;

    brkindx = (int)(TextureUtils::instance().floorInline(x) + TextureUtils::instance().floorInline(y) + TextureUtils::instance().floorInline(z));

    *&point = Vector3Dd(x, y, z);

    if (brkindx & 1) {
        colourAt(
            colour, ((Texture *)texture->color1), &point,
            smallTolerance);
    } else {
        colourAt(
            colour, ((Texture *)texture->color2), &point,
            smallTolerance);
    }
}

/**
Color gradient texture: uses fractional x, y, or z based on which axis components of
textureGradient are non-zero. Requires a colour map; works best with a circular map where
value 1.001 matches value 0.0. Concept from DBW Render, extended to all three axes.
*/
void
ColorTextureFixture::gradient(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    RGBAColor newColour;
    double value = 0.0;
    double turb;
    Vector3Dd gradTurbulence;

    if ((turb = texture->turbulence) != 0.0) {
        TextureUtils::instance().DTurbulence(&gradTurbulence, x, y, z, texture->octaves);
        x += gradTurbulence.x() * turb;
        y += gradTurbulence.y() * turb;
        z += gradTurbulence.z() * turb;
    }

    if (texture->colorMap == nullptr) {
        return;
    }
    if (texture->textureGradient.x() != 0.0) {
        x = TextureUtils::instance().fabsInline(x);
        value += x - TextureUtils::instance().floorInline(x); // obtain fractional X component
    }
    if (texture->textureGradient.y() != 0.0) {
        y = TextureUtils::instance().fabsInline(y);
        value += y - TextureUtils::instance().floorInline(y); // obtain fractional Y component
    }
    if (texture->textureGradient.z() != 0.0) {
        z = TextureUtils::instance().fabsInline(z);
        value += z - TextureUtils::instance().floorInline(z); // obtain fractional Z component
    }
    value = ((value > 1.0) ? fmod(value, 1.0) : value); // clamp to 1.0


    TextureUtils::instance().computeColour(&newColour, texture->colorMap, value);
    colour->Red += newColour.Red;
    colour->Green += newColour.Green;
    colour->Blue += newColour.Blue;
    colour->Alpha += newColour.Alpha;
}

/**
[PERL1985].290 - Granite: 1/f fractal composition of Noise() over octaves.
Union of spotted and dented textures using a 1/f fractal noise for color values.
Typically used with small scaling values; works with colour maps for pink granite.
*/
void
ColorTextureFixture::granite(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    int i;
    double temp;
    double noise = 0.0;
    double freq = 1.0;
    RGBAColor newColour;

    for (i = 0; i < 6; freq *= 2.0, i++) {
        temp =
            0.5 - TextureUtils::instance().Noise(x * 4 * freq, y * 4 * freq, z * 4 * freq);
        temp = TextureUtils::instance().fabsInline(temp);
        noise += temp / freq;
    }


    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColour(&newColour, texture->colorMap, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += noise; // white (1.0) * noise
    colour->Green += noise;
    colour->Blue += noise;
}

/** [PERL1985].291 - Marble: sine-wave color splining with turbulence-perturbed phase. */
// Implements: x = point[1] + turbulence(point); color = marble_color(sin(x))
void
ColorTextureFixture::marble(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    double noise;
    double hue;
    RGBAColor newColour;

    noise = TextureUtils::instance().triangleWave(
        x + TextureUtils::instance().Turbulence(x, y, z, texture->octaves) *
                texture->turbulence);

    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColour(&newColour, texture->colorMap, noise);
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

/**
[PERL1985].290 - Spotted: basic noise-based random surface texture (Spotted Donut example).
Implements: color = white * Noise(point). With reflectivity can look like organ pipe metal;
with tiny scaling values, like masonry or concrete.
*/
void
ColorTextureFixture::spotted(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    double noise;
    RGBAColor newColour;

    noise = TextureUtils::instance().Noise(x, y, z);


    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColour(&newColour, texture->colorMap, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += noise; // white (1.0) * noise
    colour->Green += noise;
    colour->Blue += noise;
}

/** [PERL1985].291 - Wood: turbulence-based ring patterns via periodic wave on radial distance. */
// Uses DTurbulence gradient perturbation + cycloidal() for ring bands
void
ColorTextureFixture::wood(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    double noise;
    double length;
    Vector3Dd woodTurbulence;
    Vector3Dd point;
    RGBAColor newColour;

    TextureUtils::instance().DTurbulence(&woodTurbulence, x, y, z, texture->octaves);


    double pointX =
        TextureUtils::instance().cycloidal((x + woodTurbulence.x()) * texture->turbulence);
    double pointY =
        TextureUtils::instance().cycloidal((y + woodTurbulence.y()) * texture->turbulence);

    pointX += x;
    pointY += y;
    point = Vector3Dd(pointX, pointY, 0.0);
    length = point.length();
    noise = TextureUtils::instance().triangleWave(length);

    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColour(&newColour, texture->colorMap, noise);
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

/** Leopard texture by Scott Taylor, SWT 7/18/91. */
void
ColorTextureFixture::leopard(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    // The variable noise is not used as noise in this function
    double noise;
    double turb;
    double temp1;
    double temp2;
    double temp3;
    RGBAColor newColour;
    Vector3Dd leopardTurbulence;


    if ((turb = texture->turbulence) != 0.0) {
        TextureUtils::instance().DTurbulence(
            &leopardTurbulence, x, y, z, texture->octaves);
        x += leopardTurbulence.x() * turb;
        y += leopardTurbulence.y() * turb;
        z += leopardTurbulence.z() * turb;
    }
    // This form didn't work with Zortech 386 compiler:
    // noise = (((sin(x)+sin(y)+sin(z))/3)*((sin(x)+sin(y)+sin(z))/3));
    temp1 = sin(x);
    temp2 = sin(y);
    temp3 = sin(z);
    noise = (((temp1 + temp2 + temp3) / 3)*((temp1 + temp2 + temp3) / 3));



    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColour(&newColour, texture->colorMap, noise);
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

/** Onion texture by Scott Taylor, SWT 7/18/91. */
void
ColorTextureFixture::onion(
    double x, double y, double z, Texture *texture, RGBAColor *colour)
{
    // The variable noise is not used as noise in this function
    double noise;
    double turb;
    RGBAColor newColour;
    Vector3Dd onionTurbulence;


    if ((turb = texture->turbulence) != 0.0) {
        TextureUtils::instance().DTurbulence(&onionTurbulence, x, y, z, texture->octaves);
        x += onionTurbulence.x() * turb;
        y += onionTurbulence.y() * turb;
        z += onionTurbulence.z() * turb;
    }

    // Alternative ramp 0-1,1-0,0-1,1-0...:
    // noise = (fmod(sqrt(x*x+y*y+z*z), 2.0) - 1.0);
    // if (noise < 0.0) { noise = 0.0 - noise; }

    // This ramp goes 0-1,0-1,0-1,0-1...
    noise = (fmod(
        std::sqrt(((x)*(x)) + ((y)*(y)) + ((z)*(z))),
        1.0));

    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColour(&newColour, texture->colorMap, noise);
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
