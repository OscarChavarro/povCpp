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
colorTextureFixture::colorAt(
    RGBAColor *color, Texture *texture, Vector3Dd *intersectionPoint, double smallTolerance)
{
    double x;
    double y;
    double z;
    Vector3Dd transformedPoint;
    mapTextureFixture mapFixture;
    textureFixture textureFixture;

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
        Color::makeColor(color, 0.0, 0.0, 0.0);
        color->Alpha = 0.0;
        break;

    case (int)SolidTextureColorTextures::COLOUR_TEXTURE:
        color->Red += texture->color1->Red;
        color->Green += texture->color1->Green;
        color->Blue += texture->color1->Blue;
        color->Alpha += texture->color1->Alpha;
        break;

    case (int)SolidTextureColorTextures::BOZO_TEXTURE:
        bozo(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::MARBLE_TEXTURE:
        marble(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::WOOD_TEXTURE:
        wood(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::BRICK_TEXTURE:
        brick(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::CHECKER_TEXTURE:
        checker(x, y, z, texture, color, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE:
        checkerTexture(x, y, z, texture, color, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::SPOTTED_TEXTURE:
        spotted(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::AGATE_TEXTURE:
        agate(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::GRANITE_TEXTURE:
        granite(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::GRADIENT_TEXTURE:
        gradient(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::IMAGEMAP_TEXTURE:
        mapFixture.imageMap(x, y, z, texture, color, smallTolerance);
        break;

    case (int)SolidTextureColorTextures::ONION_TEXTURE:
        onion(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::LEOPARD_TEXTURE:
        leopard(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::PAINTED1_TEXTURE:
        textureFixture.painted1(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::PAINTED2_TEXTURE:
        textureFixture.painted2(x, y, z, texture, color);
        break;

    case (int)SolidTextureColorTextures::PAINTED3_TEXTURE:
        textureFixture.painted3(x, y, z, texture, color);
        break;
    }
}

/** [PERL1985].290 - Agate: combines turbulence modulation with periodic wave function. */
void
colorTextureFixture::agate(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    double noise;
    double hue;
    RGBAColor newColor;

    noise = textureUtils::instance().cycloidal(
                1.3 * textureUtils::instance().Turbulence(x, y, z, texture->octaves) +
                1.1 * z) +
            1;
    noise *= 0.5;
    noise = pow(noise, 0.77);


    if (texture->colorMap != nullptr) {
        textureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->Red += newColor.Red;
        color->Green += newColor.Green;
        color->Blue += newColor.Blue;
        color->Alpha += newColor.Alpha;
        return;
    }

    hue = 1.0 - noise;

    if (noise < 0.5) {
        color->Red += (1.0 - (noise / 10));
        color->Green += (1.0 - (noise / 5));
        color->Blue += hue;
    } else if (noise < 0.6) {
        color->Red += 0.9;
        color->Green += 0.7;
        color->Blue += hue;
    } else {
        color->Red += (0.6 + hue);
        color->Green += (0.3 + hue);
        color->Blue += hue;
    }
}

/** [PERL1985].290 - Bozo: displaced Noise() via DTurbulence gradient perturbation. */
void
colorTextureFixture::bozo(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    double noise;
    double turb;
    RGBAColor newColor;
    Vector3Dd bozoTurbulence;

    if ((turb = texture->turbulence) != 0.0) {
        textureUtils::instance().DTurbulence(&bozoTurbulence, x, y, z, texture->octaves);
        x += bozoTurbulence.x() * turb;
        y += bozoTurbulence.y() * turb;
        z += bozoTurbulence.z() * turb;
    }

    noise = textureUtils::instance().Noise(x, y, z);

    if (texture->colorMap != nullptr) {
        textureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->Red += newColor.Red;
        color->Green += newColor.Green;
        color->Blue += newColor.Blue;
        color->Alpha += newColor.Alpha;
        return;
    }

    if (noise < 0.4) {
        color->Red += 1.0;
        color->Green += 1.0;
        color->Blue += 1.0;
        return;
    }

    if (noise < 0.6) {
        color->Green += 1.0;
        return;
    }

    if (noise < 0.8) {
        color->Blue += 1.0;
        return;
    }

    color->Red += 1.0;
}

void
colorTextureFixture::brick(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    double xr;
    double yr;
    double zr;

    xr = fabs(fmod(x, 1.0));
    yr = fabs(fmod(y, 1.0));
    zr = fabs(fmod(z, 1.0));
    *color = *texture->color2;

    if (xr > 0 && xr < texture->mortar) {
        *color = *texture->color1;
        return;
    }
    if (yr > 0 && yr < texture->mortar) {
        *color = *texture->color1;
        return;
    }
    if (zr > 0 && zr < texture->mortar) {
        *color = *texture->color1;
    }
}

void
colorTextureFixture::checker(double x, double y, double z, Texture *texture,
    RGBAColor *color, double smallTolerance)
{
    int brkindx;

    x += smallTolerance; // add a small offset to x, y, z, axes to preventd noise
    y += smallTolerance;
    z += smallTolerance;

    // AAC: was just x + z
    // AAC: GeometryConstants::Small_Tolerance added to get around Microsoft C (int) bug
    brkindx = (int)(textureUtils::instance().floorInline(x) + textureUtils::instance().floorInline(y) + textureUtils::instance().floorInline(z));

    if (brkindx & 1) {
        color->Red += texture->color1->Red;
        color->Green += texture->color1->Green;
        color->Blue += texture->color1->Blue;
        color->Alpha += texture->color1->Alpha;
    } else {
        color->Red += texture->color2->Red;
        color->Green += texture->color2->Green;
        color->Blue += texture->color2->Blue;
        color->Alpha += texture->color2->Alpha;
    }
}

void
colorTextureFixture::checkerTexture(double x, double y, double z,
    Texture *texture, RGBAColor *color,
    double smallTolerance)
{
    int brkindx;
    Vector3Dd point;

    x += smallTolerance; // add a small offset to x, y, z, axes to prevent noise
    y += smallTolerance;
    z += smallTolerance;

    brkindx = (int)(textureUtils::instance().floorInline(x) + textureUtils::instance().floorInline(y) + textureUtils::instance().floorInline(z));

    *&point = Vector3Dd(x, y, z);

    if (brkindx & 1) {
        colorAt(
            color, ((Texture *)texture->color1), &point,
            smallTolerance);
    } else {
        colorAt(
            color, ((Texture *)texture->color2), &point,
            smallTolerance);
    }
}

/**
Color gradient texture: uses fractional x, y, or z based on which axis components of
textureGradient are non-zero. Requires a color map; works best with a circular map where
value 1.001 matches value 0.0. Concept from DBW Render, extended to all three axes.
*/
void
colorTextureFixture::gradient(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    RGBAColor newColor;
    double value = 0.0;
    double turb;
    Vector3Dd gradTurbulence;

    if ((turb = texture->turbulence) != 0.0) {
        textureUtils::instance().DTurbulence(&gradTurbulence, x, y, z, texture->octaves);
        x += gradTurbulence.x() * turb;
        y += gradTurbulence.y() * turb;
        z += gradTurbulence.z() * turb;
    }

    if (texture->colorMap == nullptr) {
        return;
    }
    if (texture->textureGradient.x() != 0.0) {
        x = textureUtils::instance().fabsInline(x);
        value += x - textureUtils::instance().floorInline(x); // obtain fractional X component
    }
    if (texture->textureGradient.y() != 0.0) {
        y = textureUtils::instance().fabsInline(y);
        value += y - textureUtils::instance().floorInline(y); // obtain fractional Y component
    }
    if (texture->textureGradient.z() != 0.0) {
        z = textureUtils::instance().fabsInline(z);
        value += z - textureUtils::instance().floorInline(z); // obtain fractional Z component
    }
    value = ((value > 1.0) ? fmod(value, 1.0) : value); // clamp to 1.0


    textureUtils::instance().computeColor(&newColor, texture->colorMap, value);
    color->Red += newColor.Red;
    color->Green += newColor.Green;
    color->Blue += newColor.Blue;
    color->Alpha += newColor.Alpha;
}

/**
[PERL1985].290 - Granite: 1/f fractal composition of Noise() over octaves.
Union of spotted and dented textures using a 1/f fractal noise for color values.
Typically used with small scaling values; works with color maps for pink granite.
*/
void
colorTextureFixture::granite(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    int i;
    double temp;
    double noise = 0.0;
    double freq = 1.0;
    RGBAColor newColor;

    for (i = 0; i < 6; freq *= 2.0, i++) {
        temp =
            0.5 - textureUtils::instance().Noise(x * 4 * freq, y * 4 * freq, z * 4 * freq);
        temp = textureUtils::instance().fabsInline(temp);
        noise += temp / freq;
    }


    if (texture->colorMap != nullptr) {
        textureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->Red += newColor.Red;
        color->Green += newColor.Green;
        color->Blue += newColor.Blue;
        color->Alpha += newColor.Alpha;
        return;
    }

    color->Red += noise; // white (1.0) * noise
    color->Green += noise;
    color->Blue += noise;
}

/** [PERL1985].291 - Marble: sine-wave color splining with turbulence-perturbed phase. */
// Implements: x = point[1] + turbulence(point); color = marble_color(sin(x))
void
colorTextureFixture::marble(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    double noise;
    double hue;
    RGBAColor newColor;

    noise = textureUtils::instance().triangleWave(
        x + textureUtils::instance().Turbulence(x, y, z, texture->octaves) *
                texture->turbulence);

    if (texture->colorMap != nullptr) {
        textureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->Red += newColor.Red;
        color->Green += newColor.Green;
        color->Blue += newColor.Blue;
        color->Alpha += newColor.Alpha;
        return;
    }

    if (noise < 0.0) {
        color->Red += 0.9;
        color->Green += 0.8;
        color->Blue += 0.8;
    } else if (noise < 0.9) {
        color->Red += 0.9;
        hue = 0.8 - noise * 0.8;
        color->Green += hue;
        color->Blue += hue;
    }
}

/**
[PERL1985].290 - Spotted: basic noise-based random surface texture (Spotted Donut example).
Implements: color = white * Noise(point). With reflectivity can look like organ pipe metal;
with tiny scaling values, like masonry or concrete.
*/
void
colorTextureFixture::spotted(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    double noise;
    RGBAColor newColor;

    noise = textureUtils::instance().Noise(x, y, z);


    if (texture->colorMap != nullptr) {
        textureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->Red += newColor.Red;
        color->Green += newColor.Green;
        color->Blue += newColor.Blue;
        color->Alpha += newColor.Alpha;
        return;
    }

    color->Red += noise; // white (1.0) * noise
    color->Green += noise;
    color->Blue += noise;
}

/** [PERL1985].291 - Wood: turbulence-based ring patterns via periodic wave on radial distance. */
// Uses DTurbulence gradient perturbation + cycloidal() for ring bands
void
colorTextureFixture::wood(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    double noise;
    double length;
    Vector3Dd woodTurbulence;
    Vector3Dd point;
    RGBAColor newColor;

    textureUtils::instance().DTurbulence(&woodTurbulence, x, y, z, texture->octaves);


    double pointX =
        textureUtils::instance().cycloidal((x + woodTurbulence.x()) * texture->turbulence);
    double pointY =
        textureUtils::instance().cycloidal((y + woodTurbulence.y()) * texture->turbulence);

    pointX += x;
    pointY += y;
    point = Vector3Dd(pointX, pointY, 0.0);
    length = point.length();
    noise = textureUtils::instance().triangleWave(length);

    if (texture->colorMap != nullptr) {
        textureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->Red += newColor.Red;
        color->Green += newColor.Green;
        color->Blue += newColor.Blue;
        color->Alpha += newColor.Alpha;
        return;
    }

    if (noise > 0.6) {
        color->Red += 0.4;
        color->Green += 0.133;
        color->Blue += 0.066;
    } else {
        color->Red += 0.666;
        color->Green += 0.312;
        color->Blue += 0.2;
    }
}

/** Leopard texture by Scott Taylor, SWT 7/18/91. */
void
colorTextureFixture::leopard(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    // The variable noise is not used as noise in this function
    double noise;
    double turb;
    double temp1;
    double temp2;
    double temp3;
    RGBAColor newColor;
    Vector3Dd leopardTurbulence;


    if ((turb = texture->turbulence) != 0.0) {
        textureUtils::instance().DTurbulence(
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
        textureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->Red += newColor.Red;
        color->Green += newColor.Green;
        color->Blue += newColor.Blue;
        color->Alpha += newColor.Alpha;
        return;
    }

    color->Red += noise;
    color->Green += noise;
    color->Blue += noise;
}

/** Onion texture by Scott Taylor, SWT 7/18/91. */
void
colorTextureFixture::onion(
    double x, double y, double z, Texture *texture, RGBAColor *color)
{
    // The variable noise is not used as noise in this function
    double noise;
    double turb;
    RGBAColor newColor;
    Vector3Dd onionTurbulence;


    if ((turb = texture->turbulence) != 0.0) {
        textureUtils::instance().DTurbulence(&onionTurbulence, x, y, z, texture->octaves);
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
        textureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->Red += newColor.Red;
        color->Green += newColor.Green;
        color->Blue += newColor.Blue;
        color->Alpha += newColor.Alpha;
        return;
    }

    color->Red += noise;
    color->Green += noise;
    color->Blue += noise;
}
