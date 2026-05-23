/****************************************************************************
 *                     txtbump.c
 *
 *  This module implements solid texturing functions that perturb the surface
 *  normal to create a bumpy effect.
 *
 *****************************************************************************/
/*
Some texture ideas garnered from SIGGRAPH '85 Volume 19 Number 3,
"An Image Synthesizer" By Ken Perlin.
Further Ideas Garnered from "The RenderMan Companion" (Addison Wesley)
*/

#include "media/BumpTextureFixture.h"
#include "common/logger/Logger.h"
#include "common/LegacyBoolean.h"
#include <cstdio>
#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

void
BumpTextureFixture::ripples(
    double x, double y, double z, Texture *texture, Vector3Dd *normal, int debugEnabled)
{
    int i;
    Vector3Dd point;
    double length;
    double scalar;
    double index;

    if (debugEnabled) {
        Logger::info("ripples %g %g %g", x, y, z);
    }

    for (i = 0; i < NUMBER_OF_WAVES; i++) {
        point.x = x;
        point.y = y;
        point.z = z;
        point.sub(TextureUtils::waveSources()[i]);
        length = point.dotProduct(point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = length * texture->Frequency + texture->Phase;
        scalar = TextureUtils::cycloidal(index) * texture->bumpAmount;

        if (debugEnabled) {
            Logger::info(" index %g scalar %g length %g\n", index, scalar, length);
        }

        VectorOps::vScale(
            point, point, scalar / length / (double)NUMBER_OF_WAVES);
        (*normal).add(point);
    }
    (*normal).normalize();
}

void
BumpTextureFixture::waves(
    double x, double y, double z, Texture *texture, Vector3Dd *normal, int debugEnabled)
{
    int i;
    Vector3Dd point;
    double length;
    double scalar;
    double index;
    double sinValue;

    if (debugEnabled) {
        Logger::info("waves %g %g %g\n", x, y, z);
    }

    for (i = 0; i < NUMBER_OF_WAVES; i++) {
        point.x = x;
        point.y = y;
        point.z = z;
        point.sub(TextureUtils::waveSources()[i]);
        length = point.dotProduct(point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = (length * texture->Frequency * TextureUtils::waveFrequency()[i]) + texture->Phase;
        sinValue = TextureUtils::cycloidal(index);

        scalar = sinValue * texture->bumpAmount / TextureUtils::waveFrequency()[i];
        VectorOps::vScale(
            point, point, scalar / length / (double)NUMBER_OF_WAVES);
        (*normal).add(point);
    }
    (*normal).normalize();
}

void
BumpTextureFixture::bumps(
    double x, double y, double z, Texture *texture, Vector3Dd *normal, int debugEnabled)
{
    Vector3Dd bumpTurb;

    if (texture->bumpAmount == 0.0) {
        return; /* why are we here?? */
    }

    if (debugEnabled) {
        Logger::info("bumps %g %g %g\n", x, y, z);
    }

    TextureUtils::DNoise(&bumpTurb, x, y, z); /* Get Normal Displacement Val. */
    bumpTurb.scale(texture->bumpAmount);
    (*normal).add(bumpTurb); /* displace "normal" */
    (*normal).normalize();   /* normalize normal! */
}

/*
dents is similar to bumps, but uses noise() to control the amount of
dnoise() perturbation of the object normal...
*/
void
BumpTextureFixture::dents(
    double x, double y, double z, Texture *texture, Vector3Dd *normal, int debugEnabled)
{
    Vector3Dd stuccoTurb;
    double noise;

    if (texture->bumpAmount == 0.0) {
        return; /* why are we here?? */
    }

    noise = TextureUtils::Noise(x, y, z);

    noise = noise * noise * noise * texture->bumpAmount;

    if (debugEnabled) {
        Logger::info("dents %g %g %g noise %g\n", x, y, z, noise);
    }

    TextureUtils::DNoise(
        &stuccoTurb, x, y, z); /* Get Normal Displacement Val. */

    stuccoTurb.scale(noise);
    (*normal).add(stuccoTurb); /* displace "normal" */
    (*normal).normalize();     /* normalize normal! */
}

/*
    Ideas garnered from the April 89 Byte Graphics Supplement on RenderMan,
    refined from "The RenderMan Companion, by Steve Upstill of Pixar, (C) 1990
    Addison-Wesley.
*/

/*
    wrinkles - This is my implementation of the dented() routine, using
    a surface iterative fractal derived from DTurbulence.  This is a 3-D vers.
    (thanks to TextureUtils::DNoise()...) of the usual version using the
   singular TextureUtils::Noise()... Seems to look a lot like wrinkles,
   however... (hmmm)
*/

void
BumpTextureFixture::wrinkles(
    double x, double y, double z, Texture *texture, Vector3Dd *normal, int debugEnabled)
{
    int i;
    double scale = 1.0;
    Vector3Dd result;
    Vector3Dd value;

    if (texture->bumpAmount == 0.0) {
        return; /* why are we here?? */
    }

    if (debugEnabled) {
        Logger::info("wrinkles %g %g %g\n", x, y, z);
    }

    result.x = 0.0;
    result.y = 0.0;
    result.z = 0.0;

    for (i = 0; i < 10; scale *= 2.0, i++) {
        TextureUtils::DNoise(
            &value, x * scale, y * scale, z * scale); /* * scale,*/
        result.x += fabsInline(value.x / scale);
        result.y += fabsInline(value.y / scale);
        result.z += fabsInline(value.z / scale);
    }

    result.scale(texture->bumpAmount);
    (*normal).add(result); /* displace "normal" */
    (*normal).normalize(); /* normalize normal! */
}
