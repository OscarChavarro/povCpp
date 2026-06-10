/**
Implements solid texturing functions that perturb the surface normal to create bumpy effects.

References:
[PERL1985] "An Image Synthesizer" (SIGGRAPH '85, Vol. 19 No. 3, pp. 287-296).
"The RenderMan Companion" (Addison Wesley).
*/

#include <cstdio>
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "solidTexture/BumpTextureFixture.h"
#include "solidTexture/ProceduralNoise.h"
#include "solidTexture/TextureUtils.h"

BumpTextureFixture::BumpTextureFixture(ProceduralNoise *proceduralNoise)
    : proceduralNoise(proceduralNoise)
{
}

/**
[PERL1985].291-292 - Ripples: superimposed wave fronts from point sources.
Implements: normal += wave(point - center), wave(v) = direction(v) * cycloid(norm(v)).
*/
void
BumpTextureFixture::ripples(
    double x, double y, double z, double bumpAmount, double frequency,
    double phase, int numberOfWaves, Vector3Dd *normal)
{
    int i;
    Vector3Dd point;
    double length;
    double scalar;
    double index;

    for (i = 0; i < numberOfWaves; i++) {
        point = Vector3Dd(x, y, z);
        point = point.subtract(TextureUtils::instance().waveSources()[i]);
        length = point.dotProduct(point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = length * frequency + phase;
        scalar = proceduralNoise->cycloidal(index) * bumpAmount;

        point = point.multiply(scalar / length / (double)numberOfWaves);
        *normal = normal->add(point);
    }
    *normal = (*normal).normalizedFast();
}

/**
[PERL1985].291-292 - Waves: superimposed wave fronts with 1/f frequency distribution.
Implements: wave(v) = direction((point-c)*f)/f with per-source frequency scaling.
*/
void
BumpTextureFixture::waves(
    double x, double y, double z, double bumpAmount, double frequency,
    double phase, int numberOfWaves, Vector3Dd *normal)
{
    int i;
    Vector3Dd point;
    double length;
    double scalar;
    double index;
    double sinValue;

    for (i = 0; i < numberOfWaves; i++) {
        point = Vector3Dd(x, y, z);
        point = point.subtract(TextureUtils::instance().waveSources()[i]);
        length = point.dotProduct(point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = (length * frequency * TextureUtils::instance().waveFrequency()[i]) + phase;
        sinValue = proceduralNoise->cycloidal(index);

        scalar = sinValue * bumpAmount / TextureUtils::instance().waveFrequency()[i];
        point = point.multiply(scalar / length / (double)numberOfWaves);
        *normal = normal->add(point);
    }
    *normal = (*normal).normalizedFast();
}

/**
[PERL1985].290 - Bumps: direct DNoise gradient-based normal perturbation (Bumpy Donut example).
Implements: normal += DNoise(point).
*/
void
BumpTextureFixture::bumps(
    double x, double y, double z, double bumpAmount, Vector3Dd *normal)
{
    Vector3Dd bumpTurb;

    if (bumpAmount == 0.0) {
        return; // why are we here?
    }


    proceduralNoise->dNoise(&bumpTurb, x, y, z); // Get Normal Displacement value
    bumpTurb = bumpTurb.multiply(bumpAmount);
    *normal = normal->add(bumpTurb); // displace "normal"
    *normal = (*normal).normalizedFast(); // normalize normal!
}

/**
[PERL1985].290 - Dents: Noise() modulated DNoise() gradient perturbation.
Similar to bumps but uses Noise() to gate the amount of DNoise() perturbation.
*/
void
BumpTextureFixture::dents(
    double x, double y, double z, double bumpAmount, Vector3Dd *normal)
{
    Vector3Dd stuccoTurb;
    double noise;

    if (bumpAmount == 0.0) {
        return; // why are we here?
    }

    noise = proceduralNoise->noise(x, y, z);

    noise = noise * noise * noise * bumpAmount;


    proceduralNoise->dNoise(&stuccoTurb, x, y, z); // Get Normal Displacement value

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
BumpTextureFixture::wrinkles(
    double x, double y, double z, double bumpAmount, Vector3Dd *normal)
{
    int i;
    double scale = 1.0;
    Vector3Dd result;
    Vector3Dd value;

    if (bumpAmount == 0.0) {
        return; // why are we here?
    }

    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;

    for (i = 0; i < 10; scale *= 2.0, i++) {
        proceduralNoise->dNoise(&value, x * scale, y * scale, z * scale); // scale
        rx += TextureUtils::instance().fabsInline(value.x() / scale);
        ry += TextureUtils::instance().fabsInline(value.y() / scale);
        rz += TextureUtils::instance().fabsInline(value.z() / scale);
    }
    result = Vector3Dd(rx, ry, rz);

    result = result.multiply(bumpAmount);
    *normal = normal->add(result); // displace "normal"
    *normal = (*normal).normalizedFast(); // normalize normal!
}
