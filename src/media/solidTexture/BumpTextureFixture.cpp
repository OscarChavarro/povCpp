/**
Implements solid texturing functions that perturb the surface normal to create bumpy effects.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
"The RenderMan Companion" (Addison Wesley).
*/

#include <cstdio>
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "media/solidTexture/BumpTextureFixture.h"
#include "media/solidTexture/Texture.h"

/**
[PERL1985].291-292 - Ripples: superimposed wave fronts from point sources.
Implements: normal += wave(point - center), wave(v) = direction(v) * cycloid(norm(v)).
*/
void
bumpTextureFixture::ripples(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
    int i;
    Vector3Dd point;
    double length;
    double scalar;
    double index;

    for (i = 0; i < Texture::NUMBER_OF_WAVES; i++) {
        point = Vector3Dd(x, y, z);
        point = point.subtract(textureUtils::instance().waveSources()[i]);
        length = point.dotProduct(point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = length * texture->frequency + texture->phase;
        scalar = textureUtils::instance().cycloidal(index) * texture->bumpAmount;

        point = point.multiply(scalar / length / (double)Texture::NUMBER_OF_WAVES);
        *normal = normal->add(point);
    }
    *normal = (*normal).normalizedFast();
}

/**
[PERL1985].291-292 - Waves: superimposed wave fronts with 1/f frequency distribution.
Implements: wave(v) = direction((point-c)*f)/f with per-source frequency scaling.
*/
void
bumpTextureFixture::waves(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
    int i;
    Vector3Dd point;
    double length;
    double scalar;
    double index;
    double sinValue;

    for (i = 0; i < Texture::NUMBER_OF_WAVES; i++) {
        point = Vector3Dd(x, y, z);
        point = point.subtract(textureUtils::instance().waveSources()[i]);
        length = point.dotProduct(point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = (length * texture->frequency * textureUtils::instance().waveFrequency()[i]) + texture->phase;
        sinValue = textureUtils::instance().cycloidal(index);

        scalar = sinValue * texture->bumpAmount / textureUtils::instance().waveFrequency()[i];
        point = point.multiply(scalar / length / (double)Texture::NUMBER_OF_WAVES);
        *normal = normal->add(point);
    }
    *normal = (*normal).normalizedFast();
}

/**
[PERL1985].290 - Bumps: direct DNoise gradient-based normal perturbation (Bumpy Donut example).
Implements: normal += DNoise(point).
*/
void
bumpTextureFixture::bumps(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
    Vector3Dd bumpTurb;

    if (texture->bumpAmount == 0.0) {
        return; // why are we here?
    }


    textureUtils::instance().DNoise(&bumpTurb, x, y, z); // Get Normal Displacement value
    bumpTurb = bumpTurb.multiply(texture->bumpAmount);
    *normal = normal->add(bumpTurb); // displace "normal"
    *normal = (*normal).normalizedFast(); // normalize normal!
}

/**
[PERL1985].290 - Dents: Noise() modulated DNoise() gradient perturbation.
Similar to bumps but uses Noise() to gate the amount of DNoise() perturbation.
*/
void
bumpTextureFixture::dents(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
    Vector3Dd stuccoTurb;
    double noise;

    if (texture->bumpAmount == 0.0) {
        return; // why are we here?
    }

    noise = textureUtils::instance().Noise(x, y, z);

    noise = noise * noise * noise * texture->bumpAmount;


    textureUtils::instance().DNoise(&stuccoTurb, x, y, z); // Get Normal Displacement value

    stuccoTurb = stuccoTurb.multiply(noise);
    *normal = normal->add(stuccoTurb); // displace "normal"
    *normal = (*normal).normalizedFast(); // normalize normal!
}

/**
[PERL1985].290,Appendix - Wrinkles: 1/f fractal composition of DNoise() over octaves.
Vector-valued turbulence: sum over octaves of |DNoise(p*scale)| / scale.
3-D surface iterative fractal derived from DTurbulence; from "The RenderMan Companion"
(Addison Wesley / Steve Upstill of Pixar, 1990) and the April 89 Byte RenderMan supplement.
*/
void
bumpTextureFixture::wrinkles(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
    int i;
    double scale = 1.0;
    Vector3Dd result;
    Vector3Dd value;

    if (texture->bumpAmount == 0.0) {
        return; // why are we here?
    }

    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;

    for (i = 0; i < 10; scale *= 2.0, i++) {
        textureUtils::instance().DNoise(&value, x * scale, y * scale, z * scale); // scale
        rx += textureUtils::instance().fabsInline(value.x() / scale);
        ry += textureUtils::instance().fabsInline(value.y() / scale);
        rz += textureUtils::instance().fabsInline(value.z() / scale);
    }
    result = Vector3Dd(rx, ry, rz);

    result = result.multiply(texture->bumpAmount);
    *normal = normal->add(result); // displace "normal"
    *normal = (*normal).normalizedFast(); // normalize normal!
}
