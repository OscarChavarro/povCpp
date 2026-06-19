/**
Implements solid texturing functions that modify the color of an object's surface.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
[UPST1990] "The RenderMan Companion" (Addison Wesley).
*/

#include <cmath>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ColorTextureFixture.h"
#include "vsdk/toolkit/media/solidTexture/procedural/ProceduralNoise.h"
#include "vsdk/toolkit/media/solidTexture/TextureUtils.h"

ColorTextureFixture::ColorTextureFixture(
    const ProceduralNoise *proceduralNoise, const TextureUtils *textureUtils)
    : proceduralNoise(proceduralNoise), textureUtils(textureUtils)
{
}

/**
[PERL1985].290 - Agate: combines turbulence modulation with periodic wave function.
*/
void
ColorTextureFixture::agate(double x, double y, double z, int octaves, const RGBAColorPalette *colorMap, ColorRgba *color) const
{
    double noise;
    double hue;
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);

    noise = proceduralNoise->cycloidal(
                1.3 * proceduralNoise->turbulence(x, y, z, octaves) +
                1.1 * z) +
            1;
    noise *= 0.5;
    noise = java::Math::pow(noise, 0.77);


    if (colorMap != nullptr) {
        textureUtils->computeColor(&newColor, colorMap, noise);
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

/**
[PERL1985].290 - Bozo: displaced Noise() via DTurbulence gradient perturbation.
*/
void
ColorTextureFixture::bozo(
    double x, double y, double z, double turbulence, int octaves,
    const RGBAColorPalette *colorMap, ColorRgba *color) const
{
    double noise;
    double currentTurbulence;
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);
    Vector3Dd bozoTurbulence;

    if ((currentTurbulence = turbulence) != 0.0) {
        proceduralNoise->differentialTurbulence(&bozoTurbulence, x, y, z, octaves);
        x += bozoTurbulence.x() * currentTurbulence;
        y += bozoTurbulence.y() * currentTurbulence;
        z += bozoTurbulence.z() * currentTurbulence;
    }

    noise = proceduralNoise->noise(x, y, z);

    if (colorMap != nullptr) {
        textureUtils->computeColor(&newColor, colorMap, noise);
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
    double x, double y, double z, ColorRgba *color, const ColorRgba *color1,
    const ColorRgba *color2, double mortar) const
{
    double xr;
    double yr;
    double zr;

    xr = java::Math::abs(std::fmod(x, 1.0));
    yr = java::Math::abs(std::fmod(y, 1.0));
    zr = java::Math::abs(std::fmod(z, 1.0));
    *color = *color2;

    if (xr > 0 && xr < mortar) {
        *color = *color1;
        return;
    }
    if (yr > 0 && yr < mortar) {
        *color = *color1;
        return;
    }
    if (zr > 0 && zr < mortar) {
        *color = *color1;
    }
}

void
ColorTextureFixture::checker(
    double x, double y, double z, ColorRgba *color, const ColorRgba *color1,
    const ColorRgba *color2, double smallTolerance) const
{
    int index;

    x += smallTolerance; // add a small offset to x, y, z, axes to prevent differentialNoise
    y += smallTolerance;
    z += smallTolerance;

    // AAC: was just x + z
    // AAC: GeometryConstants::Small_Tolerance added to get around Microsoft C (int) bug
    index = (int)(textureUtils->floorInline(x) + textureUtils->floorInline(y) + textureUtils->floorInline(z));

    if (index & 1) {
        color->setR(color->getR() + color1->getR());
        color->setG(color->getG() + color1->getG());
        color->setB(color->getB() + color1->getB());
        color->setA(color->getA() + color1->getA());
    } else {
        color->setR(color->getR() + color2->getR());
        color->setG(color->getG() + color2->getG());
        color->setB(color->getB() + color2->getB());
        color->setA(color->getA() + color2->getA());
    }
}

/**
Color gradient texture: uses fractional x, y, or z based on which axis components of
textureGradient are non-zero. Requires a color map; works best with a circular map where
value 1.001 matches value 0.0. Concept from DBW Render, extended to all three axes.
*/
void
ColorTextureFixture::gradient(
    double x, double y, double z, double turbulence,
    const RGBAColorPalette *colorMap, Vector3Dd textureGradient, int octaves,
    ColorRgba *color) const
{
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);
    double value = 0.0;
    double currentTurbulence;
    Vector3Dd gradTurbulence;

    if ((currentTurbulence = turbulence) != 0.0) {
        proceduralNoise->differentialTurbulence(&gradTurbulence, x, y, z, octaves);
        x += gradTurbulence.x() * currentTurbulence;
        y += gradTurbulence.y() * currentTurbulence;
        z += gradTurbulence.z() * currentTurbulence;
    }

    if (colorMap == nullptr) {
        return;
    }
    if (textureGradient.x() != 0.0) {
        x = textureUtils->fabsInline(x);
        value += x - textureUtils->floorInline(x); // obtain fractional X component
    }
    if (textureGradient.y() != 0.0) {
        y = textureUtils->fabsInline(y);
        value += y - textureUtils->floorInline(y); // obtain fractional Y component
    }
    if (textureGradient.z() != 0.0) {
        z = textureUtils->fabsInline(z);
        value += z - textureUtils->floorInline(z); // obtain fractional Z component
    }
    value = ((value > 1.0) ? std::fmod(value, 1.0) : value); // clamp to 1.0


    textureUtils->computeColor(&newColor, colorMap, value);
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
    double x, double y, double z, const RGBAColorPalette *colorMap, ColorRgba *color) const
{
    int i;
    double temp;
    double noise = 0.0;
    double freq = 1.0;
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);

    for (i = 0; i < 6; freq *= 2.0, i++) {
        temp =
            0.5 - proceduralNoise->noise(x * 4 * freq, y * 4 * freq, z * 4 * freq);
        temp = textureUtils->fabsInline(temp);
        noise += temp / freq;
    }

    if (colorMap != nullptr) {
        textureUtils->computeColor(&newColor, colorMap, noise);
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

/**
[PERL1985].291 - Marble: sine-wave color splining with turbulence-perturbed phase.
Implements: x = point[1] + turbulence(point); color = marble_color(java::Math::sin(x))
*/
void
ColorTextureFixture::marble(
    double x, double y, double z, double turbulence, int octaves,
    const RGBAColorPalette *colorMap, ColorRgba *color) const
{
    double noise;
    double hue;
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);

    noise = proceduralNoise->triangleWave(
        x + proceduralNoise->turbulence(x, y, z, octaves) * turbulence);

    if (colorMap != nullptr) {
        textureUtils->computeColor(&newColor, colorMap, noise);
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
    double x, double y, double z, const RGBAColorPalette *colorMap, ColorRgba *color) const
{
    double noise = proceduralNoise->noise(x, y, z);
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);

    if (colorMap != nullptr) {
        textureUtils->computeColor(&newColor, colorMap, noise);
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

/**
[PERL1985].291 - Wood: turbulence-based ring patterns via periodic wave on radial distance.
Uses DTurbulence gradient perturbation + cycloidal() for ring bands
*/
void
ColorTextureFixture::wood(
    double x, double y, double z, double turbulence, int octaves,
    const RGBAColorPalette *colorMap, ColorRgba *color) const
{
    double noise;
    double length;
    Vector3Dd woodTurbulence;
    Vector3Dd point;
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);

    proceduralNoise->differentialTurbulence(&woodTurbulence, x, y, z, octaves);

    double pointX = proceduralNoise->cycloidal((x + woodTurbulence.x()) * turbulence) + x;
    double pointY = proceduralNoise->cycloidal((y + woodTurbulence.y()) * turbulence) + y;

    point = Vector3Dd(pointX, pointY, 0.0);
    length = point.length();
    noise = proceduralNoise->triangleWave(length);

    if (colorMap != nullptr) {
        textureUtils->computeColor(&newColor, colorMap, noise);
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
    double x, double y, double z, double turbulence, int octaves,
    const RGBAColorPalette *colorMap, ColorRgba *color) const
{
    // The variable noise is not used as noise in this function
    double noise;
    double currentTurbulence;
    double temp1;
    double temp2;
    double temp3;
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);
    Vector3Dd leopardTurbulence;

    if ((currentTurbulence = turbulence) != 0.0) {
        proceduralNoise->differentialTurbulence(
            &leopardTurbulence, x, y, z, octaves);
        x += leopardTurbulence.x() * currentTurbulence;
        y += leopardTurbulence.y() * currentTurbulence;
        z += leopardTurbulence.z() * currentTurbulence;
    }

    temp1 = java::Math::sin(x);
    temp2 = java::Math::sin(y);
    temp3 = java::Math::sin(z);
    noise = (((temp1 + temp2 + temp3) / 3)*((temp1 + temp2 + temp3) / 3));

    if (colorMap != nullptr) {
        textureUtils->computeColor(&newColor, colorMap, noise);
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

/**
Onion texture by Scott Taylor, SWT 7/18/91.
*/
void
ColorTextureFixture::onion(
    double x, double y, double z, double turbulence, int octaves,
    const RGBAColorPalette *colorMap, ColorRgba *color) const
{
    // The variable noise is not used as noise in this function
    double noise;
    double currentTurbulence;
    ColorRgba newColor(0.0, 0.0, 0.0, 0.0);
    Vector3Dd onionTurbulence;

    if ((currentTurbulence = turbulence) != 0.0) {
        proceduralNoise->differentialTurbulence(&onionTurbulence, x, y, z, octaves);
        x += onionTurbulence.x() * currentTurbulence;
        y += onionTurbulence.y() * currentTurbulence;
        z += onionTurbulence.z() * currentTurbulence;
    }

    // Alternative ramp 0-1,1-0,0-1,1-0...:
    // noise = (std::fmod(sqrt(x*x+y*y+z*z), 2.0) - 1.0);
    // if (noise < 0.0) { noise = 0.0 - noise; }

    // This ramp goes 0-1,0-1,0-1,0-1...
    noise = (std::fmod(
        java::Math::sqrt(((x)*(x)) + ((y)*(y)) + ((z)*(z))),
        1.0));

    if (colorMap != nullptr) {
        textureUtils->computeColor(&newColor, colorMap, noise);
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
