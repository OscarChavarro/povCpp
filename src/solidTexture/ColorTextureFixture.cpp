/**
Implements solid texturing functions that modify the color of an object's surface.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
"The RenderMan Companion" (Addison Wesley).
*/

#include <cstdio>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "solidTexture/ColorTextureFixture.h"
#include "solidTexture/MapTextureFixture.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/SolidTextureColorTextures.h"
#include "solidTexture/Texture.h"
#include "solidTexture/TextureFixture.h"

static constexpr double COORDINATE_LIMIT = 1.0e17;

ColorTextureFixture::ColorTextureFixture(ProceduralNoise *proceduralNoise)
    : proceduralNoise(proceduralNoise)
{
}

void
ColorTextureFixture::colorAt(
    ColorRgba *color, Texture *texture, Vector3Dd *intersectionPoint, double smallTolerance)
{
    double x;
    double y;
    double z;
    Vector3Dd transformedPoint;
    MapTextureFixture mapFixture;
    TextureFixture textureFixture(proceduralNoise);

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
        color->setR(0.0); color->setG(0.0); color->setB(0.0); color->setA(0.0);
        break;

    case (int)SolidTextureColorTextures::COLOUR_TEXTURE:
        color->setR(color->getR() + texture->color1->getR());
        color->setG(color->getG() + texture->color1->getG());
        color->setB(color->getB() + texture->color1->getB());
        color->setA(color->getA() + texture->color1->getA());
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
ColorTextureFixture::agate(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    double noise;
    double hue;
    ColorRgba newColor;

    noise = proceduralNoise->cycloidal(
                1.3 * proceduralNoise->turbulence(x, y, z, texture->octaves) +
                1.1 * z) +
            1;
    noise *= 0.5;
    noise = pow(noise, 0.77);


    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    hue = 1.0 - noise;

    if (noise < 0.5) {
        color->setR(color->getR() + (1.0 - (noise / 10)));
        color->setG(color->getG() + (1.0 - (noise / 5)));
        color->setB(color->getB() + hue);
    } else if (noise < 0.6) {
        color->setR(color->getR() + 0.9);
        color->setG(color->getG() + 0.7);
        color->setB(color->getB() + hue);
    } else {
        color->setR(color->getR() + (0.6 + hue));
        color->setG(color->getG() + (0.3 + hue));
        color->setB(color->getB() + hue);
    }
}

/** [PERL1985].290 - Bozo: displaced Noise() via DTurbulence gradient perturbation. */
void
ColorTextureFixture::bozo(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    double noise;
    double turb;
    ColorRgba newColor;
    Vector3Dd bozoTurbulence;

    if ((turb = texture->turbulence) != 0.0) {
        proceduralNoise->dTurbulence(&bozoTurbulence, x, y, z, texture->octaves);
        x += bozoTurbulence.x() * turb;
        y += bozoTurbulence.y() * turb;
        z += bozoTurbulence.z() * turb;
    }

    noise = proceduralNoise->noise(x, y, z);

    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    if (noise < 0.4) {
        color->setR(color->getR() + 1.0);
        color->setG(color->getG() + 1.0);
        color->setB(color->getB() + 1.0);
        return;
    }

    if (noise < 0.6) {
        color->setG(color->getG() + 1.0);
        return;
    }

    if (noise < 0.8) {
        color->setB(color->getB() + 1.0);
        return;
    }

    color->setR(color->getR() + 1.0);
}

void
ColorTextureFixture::brick(
    double x, double y, double z, Texture *texture, ColorRgba *color)
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
ColorTextureFixture::checker(double x, double y, double z, Texture *texture,
    ColorRgba *color, double smallTolerance)
{
    int brkindx;

    x += smallTolerance; // add a small offset to x, y, z, axes to preventd noise
    y += smallTolerance;
    z += smallTolerance;

    // AAC: was just x + z
    // AAC: GeometryConstants::Small_Tolerance added to get around Microsoft C (int) bug
    brkindx = (int)(TextureUtils::instance().floorInline(x) + TextureUtils::instance().floorInline(y) + TextureUtils::instance().floorInline(z));

    if (brkindx & 1) {
        color->setR(color->getR() + texture->color1->getR());
        color->setG(color->getG() + texture->color1->getG());
        color->setB(color->getB() + texture->color1->getB());
        color->setA(color->getA() + texture->color1->getA());
    } else {
        color->setR(color->getR() + texture->color2->getR());
        color->setG(color->getG() + texture->color2->getG());
        color->setB(color->getB() + texture->color2->getB());
        color->setA(color->getA() + texture->color2->getA());
    }
}

void
ColorTextureFixture::checkerTexture(double x, double y, double z,
    Texture *texture, ColorRgba *color,
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
ColorTextureFixture::gradient(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    ColorRgba newColor;
    double value = 0.0;
    double turb;
    Vector3Dd gradTurbulence;

    if ((turb = texture->turbulence) != 0.0) {
        proceduralNoise->dTurbulence(&gradTurbulence, x, y, z, texture->octaves);
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


    TextureUtils::instance().computeColor(&newColor, texture->colorMap, value);
    color->setR(color->getR() + newColor.getR());
    color->setG(color->getG() + newColor.getG());
    color->setB(color->getB() + newColor.getB());
    color->setA(color->getA() + newColor.getA());
}

/**
[PERL1985].290 - Granite: 1/f fractal composition of Noise() over octaves.
Union of spotted and dented textures using a 1/f fractal noise for color values.
Typically used with small scaling values; works with color maps for pink granite.
*/
void
ColorTextureFixture::granite(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    int i;
    double temp;
    double noise = 0.0;
    double freq = 1.0;
    ColorRgba newColor;

    for (i = 0; i < 6; freq *= 2.0, i++) {
        temp =
            0.5 - proceduralNoise->noise(x * 4 * freq, y * 4 * freq, z * 4 * freq);
        temp = TextureUtils::instance().fabsInline(temp);
        noise += temp / freq;
    }


    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    color->setR(color->getR() + noise); // white (1.0) * noise
    color->setG(color->getG() + noise);
    color->setB(color->getB() + noise);
}

/** [PERL1985].291 - Marble: sine-wave color splining with turbulence-perturbed phase. */
// Implements: x = point[1] + turbulence(point); color = marble_color(sin(x))
void
ColorTextureFixture::marble(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    double noise;
    double hue;
    ColorRgba newColor;

    noise = proceduralNoise->triangleWave(
        x + proceduralNoise->turbulence(x, y, z, texture->octaves) *
                texture->turbulence);

    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    if (noise < 0.0) {
        color->setR(color->getR() + 0.9);
        color->setG(color->getG() + 0.8);
        color->setB(color->getB() + 0.8);
    } else if (noise < 0.9) {
        color->setR(color->getR() + 0.9);
        hue = 0.8 - noise * 0.8;
        color->setG(color->getG() + hue);
        color->setB(color->getB() + hue);
    }
}

/**
[PERL1985].290 - Spotted: basic noise-based random surface texture (Spotted Donut example).
Implements: color = white * Noise(point). With reflectivity can look like organ pipe metal;
with tiny scaling values, like masonry or concrete.
*/
void
ColorTextureFixture::spotted(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    double noise;
    ColorRgba newColor;

    noise = proceduralNoise->noise(x, y, z);


    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    color->setR(color->getR() + noise); // white (1.0) * noise
    color->setG(color->getG() + noise);
    color->setB(color->getB() + noise);
}

/** [PERL1985].291 - Wood: turbulence-based ring patterns via periodic wave on radial distance. */
// Uses DTurbulence gradient perturbation + cycloidal() for ring bands
void
ColorTextureFixture::wood(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    double noise;
    double length;
    Vector3Dd woodTurbulence;
    Vector3Dd point;
    ColorRgba newColor;

    proceduralNoise->dTurbulence(&woodTurbulence, x, y, z, texture->octaves);


    double pointX =
        proceduralNoise->cycloidal((x + woodTurbulence.x()) * texture->turbulence);
    double pointY =
        proceduralNoise->cycloidal((y + woodTurbulence.y()) * texture->turbulence);

    pointX += x;
    pointY += y;
    point = Vector3Dd(pointX, pointY, 0.0);
    length = point.length();
    noise = proceduralNoise->triangleWave(length);

    if (texture->colorMap != nullptr) {
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    if (noise > 0.6) {
        color->setR(color->getR() + 0.4);
        color->setG(color->getG() + 0.133);
        color->setB(color->getB() + 0.066);
    } else {
        color->setR(color->getR() + 0.666);
        color->setG(color->getG() + 0.312);
        color->setB(color->getB() + 0.2);
    }
}

/** Leopard texture by Scott Taylor, SWT 7/18/91. */
void
ColorTextureFixture::leopard(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    // The variable noise is not used as noise in this function
    double noise;
    double turb;
    double temp1;
    double temp2;
    double temp3;
    ColorRgba newColor;
    Vector3Dd leopardTurbulence;


    if ((turb = texture->turbulence) != 0.0) {
        proceduralNoise->dTurbulence(
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
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    color->setR(color->getR() + noise);
    color->setG(color->getG() + noise);
    color->setB(color->getB() + noise);
}

/** Onion texture by Scott Taylor, SWT 7/18/91. */
void
ColorTextureFixture::onion(
    double x, double y, double z, Texture *texture, ColorRgba *color)
{
    // The variable noise is not used as noise in this function
    double noise;
    double turb;
    ColorRgba newColor;
    Vector3Dd onionTurbulence;


    if ((turb = texture->turbulence) != 0.0) {
        proceduralNoise->dTurbulence(&onionTurbulence, x, y, z, texture->octaves);
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
        TextureUtils::instance().computeColor(&newColor, texture->colorMap, noise);
        color->setR(color->getR() + newColor.getR());
        color->setG(color->getG() + newColor.getG());
        color->setB(color->getB() + newColor.getB());
        color->setA(color->getA() + newColor.getA());
        return;
    }

    color->setR(color->getR() + noise);
    color->setG(color->getG() + noise);
    color->setB(color->getB() + noise);
}
