/****************************************************************************
 *                     txtbump.c
 *
 *  This module implements solid texturing functions that perturb the surface
 *  normal to create a bumpy effect.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).

 *
 *****************************************************************************/
/*
Some texture ideas garnered from SIGGRAPH '85 Volume 19 Number 3,
"An Image Synthesizer" By Ken Perlin.
Further Ideas Garnered from "The RenderMan Companion" (Addison Wesley)
*/

#include <cstdio>
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/BumpTextureFixture.h"
#include "media/solidTexture/Texture.h"

// [PERL1985].291-292 - Ripples: superimposed wave fronts from point sources
// Implements: normal += wave(point - center), wave(v) = direction(v) * cycloid(norm(v))
void
BumpTextureFixture::ripples(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
    int i;
    Vector3Dd point;
    double length;
    double scalar;
    double index;

    for (i = 0; i < Texture::NUMBER_OF_WAVES; i++) {
        point = Vector3Dd(x, y, z);
        point = point.subtract(TextureUtils::instance().waveSources()[i]);
        length = point.dotProduct(point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = length * texture->Frequency + texture->Phase;
        scalar = TextureUtils::instance().cycloidal(index) * texture->bumpAmount;

        point = point.multiply(scalar / length / (double)Texture::NUMBER_OF_WAVES);
        *normal = normal->add(point);
    }
    *normal = (*normal).normalizedFast();
}

// [PERL1985].291-292 - Waves: superimposed wave fronts with 1/f frequency distribution
// Implements: wave(v) = direction((point-c)*f)/f with per-source frequency scaling
void
BumpTextureFixture::waves(
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
        point = point.subtract(TextureUtils::instance().waveSources()[i]);
        length = point.dotProduct(point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = (length * texture->Frequency * TextureUtils::instance().waveFrequency()[i]) + texture->Phase;
        sinValue = TextureUtils::instance().cycloidal(index);

        scalar = sinValue * texture->bumpAmount / TextureUtils::instance().waveFrequency()[i];
        point = point.multiply(scalar / length / (double)Texture::NUMBER_OF_WAVES);
        *normal = normal->add(point);
    }
    *normal = (*normal).normalizedFast();
}

// [PERL1985].290 - Bumps: direct DNoise gradient-based normal perturbation (Bumpy Donut example)
// Implements: normal += Dnoise(point)
void
BumpTextureFixture::bumps(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
    Vector3Dd bumpTurb;

    if (texture->bumpAmount == 0.0) {
        return; // why are we here?
    }


    TextureUtils::instance().DNoise(&bumpTurb, x, y, z); // Get Normal Displacement value
    bumpTurb = bumpTurb.multiply(texture->bumpAmount);
    *normal = normal->add(bumpTurb); // displace "normal"
    *normal = (*normal).normalizedFast(); // normalize normal!
}

// [PERL1985].290 - Dents: Noise() modulated DNoise() gradient perturbation
// Combines scalar Noise (page 289-290) to gate gradient-based displacement
/*
dents is similar to bumps, but uses noise() to control the amount of
dnoise() perturbation of the object normal...
*/
void
BumpTextureFixture::dents(
    double x, double y, double z, Texture *texture, Vector3Dd *normal)
{
    Vector3Dd stuccoTurb;
    double noise;

    if (texture->bumpAmount == 0.0) {
        return; // why are we here?
    }

    noise = TextureUtils::instance().Noise(x, y, z);

    noise = noise * noise * noise * texture->bumpAmount;


    TextureUtils::instance().DNoise(&stuccoTurb, x, y, z); // Get Normal Displacement value

    stuccoTurb = stuccoTurb.multiply(noise);
    *normal = normal->add(stuccoTurb); // displace "normal"
    *normal = (*normal).normalizedFast(); // normalize normal!
}

// [PERL1985].290,Appendix - Wrinkles: 1/f fractal composition of DNoise() over octaves
// Vector-valued turbulence: sum over octaves of |DNoise(p*scale)| / scale
/*
    Ideas garnered from the April 89 Byte Graphics Supplement on RenderMan,
    refined from "The RenderMan Companion, by Steve Upstill of Pixar, (C) 1990
    Addison-Wesley.
*/

/*
    wrinkles - This is my implementation of the dented() routine, using
    a surface iterative fractal derived from DTurbulence.  This is a 3-D vers.
    (thanks to TextureUtils::instance().DNoise()...) of the usual version using the
   singular TextureUtils::instance().Noise()... Seems to look a lot like wrinkles,
   however... (hmmm)
*/

void
BumpTextureFixture::wrinkles(
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
        TextureUtils::instance().DNoise(&value, x * scale, y * scale, z * scale); // scale
        rx += TextureUtils::instance().fabsInline(value.x() / scale);
        ry += TextureUtils::instance().fabsInline(value.y() / scale);
        rz += TextureUtils::instance().fabsInline(value.z() / scale);
    }
    result = Vector3Dd(rx, ry, rz);

    result = result.multiply(texture->bumpAmount);
    *normal = normal->add(result); // displace "normal"
    *normal = (*normal).normalizedFast(); // normalize normal!
}
