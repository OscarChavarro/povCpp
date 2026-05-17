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

#include "media/BumpTextures.h"
#include "common/FrameConfig.h"
#include "app/PovApp.h"
#include "common/Vector3D.h"
#include "common/VectorOps.h"
#include "media/Texture.h"

extern unsigned short crctab[256];

void
BumpTextures::ripples(double x, double y, double z, Texture *texture, Vector3D *normal)
{
    register int i;
    Vector3D point;
    register double length;
    register double scalar;
    register double index;

    if (Options & DEBUGGING) {
        printf("ripples %g %g %g", x, y, z);
    }

    for (i = 0; i < NUMBER_OF_WAVES; i++) {
        point.x = x;
        point.y = y;
        point.z = z;
        VectorOps::vSub(point, point, Wave_Sources[i]);
        VectorOps::vDot(length, point, point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = length * texture->Frequency + texture->Phase;
        scalar = TextureUtils::cycloidal(index) * texture->Bump_Amount;

        if (Options & DEBUGGING) {
            printf(" index %g scalar %g length %g\n", index, scalar, length);
        }

        VectorOps::vScale(point, point, scalar / length / (double)NUMBER_OF_WAVES);
        VectorOps::vAdd(*normal, *normal, point);
    }
    VectorOps::vNormalize(*normal, *normal);
}

void
BumpTextures::waves(double x, double y, double z, Texture *texture, Vector3D *normal)
{
    register int i;
    Vector3D point;
    register double length;
    register double scalar;
    register double index;
    register double sinValue;

    if (Options & DEBUGGING) {
        printf("waves %g %g %g\n", x, y, z);
    }

    for (i = 0; i < NUMBER_OF_WAVES; i++) {
        point.x = x;
        point.y = y;
        point.z = z;
        VectorOps::vSub(point, point, Wave_Sources[i]);
        VectorOps::vDot(length, point, point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = (length * texture->Frequency * frequency[i]) + texture->Phase;
        sinValue = TextureUtils::cycloidal(index);

        scalar = sinValue * texture->Bump_Amount / frequency[i];
        VectorOps::vScale(point, point, scalar / length / (double)NUMBER_OF_WAVES);
        VectorOps::vAdd(*normal, *normal, point);
    }
    VectorOps::vNormalize(*normal, *normal);
}

void
BumpTextures::bumps(double x, double y, double z, Texture *texture, Vector3D *normal)
{
    Vector3D bumpTurb;

    if (texture->Bump_Amount == 0.0) {
        return; /* why are we here?? */
    }

    if (Options & DEBUGGING) {
        printf("bumps %g %g %g\n", x, y, z);
    }

    TextureUtils::DNoise(&bumpTurb, x, y, z); /* Get Normal Displacement Val. */
    VectorOps::vScale(bumpTurb, bumpTurb, texture->Bump_Amount);
    VectorOps::vAdd(*normal, *normal, bumpTurb); /* displace "normal" */
    VectorOps::vNormalize(*normal, *normal);     /* normalize normal! */
}

/*
dents is similar to bumps, but uses noise() to control the amount of
dnoise() perturbation of the object normal...
*/
void
BumpTextures::dents(double x, double y, double z, Texture *texture, Vector3D *normal)
{
    Vector3D stuccoTurb;
    double noise;

    if (texture->Bump_Amount == 0.0) {
        return; /* why are we here?? */
    }

    noise = TextureUtils::Noise(x, y, z);

    noise = noise * noise * noise * texture->Bump_Amount;

    if (Options & DEBUGGING) {
        printf("dents %g %g %g noise %g\n", x, y, z, noise);
    }

    TextureUtils::DNoise(&stuccoTurb, x, y, z); /* Get Normal Displacement Val. */

    VectorOps::vScale(stuccoTurb, stuccoTurb, noise);
    VectorOps::vAdd(*normal, *normal, stuccoTurb); /* displace "normal" */
    VectorOps::vNormalize(*normal, *normal);       /* normalize normal! */
}

/*
    Ideas garnered from the April 89 Byte Graphics Supplement on RenderMan,
    refined from "The RenderMan Companion, by Steve Upstill of Pixar, (C) 1990
    Addison-Wesley.
*/

/*
    wrinkles - This is my implementation of the dented() routine, using
    a surface iterative fractal derived from DTurbulence.  This is a 3-D vers.
    (thanks to TextureUtils::DNoise()...) of the usual version using the singular TextureUtils::Noise()...
    Seems to look a lot like wrinkles, however... (hmmm)
*/

void
BumpTextures::wrinkles(double x, double y, double z, Texture *texture, Vector3D *normal)
{
    register int i;
    register double scale = 1.0;
    Vector3D result;
    Vector3D value;

    if (texture->Bump_Amount == 0.0) {
        return; /* why are we here?? */
    }

    if (Options & DEBUGGING) {
        printf("wrinkles %g %g %g\n", x, y, z);
    }

    result.x = 0.0;
    result.y = 0.0;
    result.z = 0.0;

    for (i = 0; i < 10; scale *= 2.0, i++) {
        TextureUtils::DNoise(&value, x * scale, y * scale, z * scale); /* * scale,*/
        result.x += fabsInline(value.x / scale);
        result.y += fabsInline(value.y / scale);
        result.z += fabsInline(value.z / scale);
    }

    VectorOps::vScale(result, result, texture->Bump_Amount);
    VectorOps::vAdd(*normal, *normal, result); /* displace "normal" */
    VectorOps::vNormalize(*normal, *normal);   /* normalize normal! */
}
