/****************************************************************************
 *                     txtbump.c
 *
 *  This module implements solid texturing functions that perturb the surface
 *  normal to create a bumpy effect.
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/
/*
Some texture ideas garnered from SIGGRAPH '85 Volume 19 Number 3,
"An Image Synthesizer" By Ken Perlin.
Further Ideas Garnered from "The RenderMan Companion" (Addison Wesley)
*/

#include "media/TxtBump.h"
#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "media/Texture.h"

extern unsigned short crctab[256];

void
ripples(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal)
{
    register int i;
    Vector3D point;
    register DBL length;
    register DBL scalar;
    register DBL index;

    if (Options & DEBUGGING) {
        printf("ripples %g %g %g", x, y, z);
    }

    for (i = 0; i < NUMBER_OF_WAVES; i++) {
        point.x = x;
        point.y = y;
        point.z = z;
        VSub(point, point, Wave_Sources[i]);
        VDot(length, point, point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = length * texture->Frequency + texture->Phase;
        scalar = cycloidal(index) * texture->Bump_Amount;

        if (Options & DEBUGGING) {
            printf(" index %g scalar %g length %g\n", index, scalar, length);
        }

        VScale(point, point, scalar / length / (DBL)NUMBER_OF_WAVES);
        VAdd(*normal, *normal, point);
    }
    VNormalize(*normal, *normal);
}

void
waves(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal)
{
    register int i;
    Vector3D point;
    register DBL length;
    register DBL scalar;
    register DBL index;
    register DBL sinValue;

    if (Options & DEBUGGING) {
        printf("waves %g %g %g\n", x, y, z);
    }

    for (i = 0; i < NUMBER_OF_WAVES; i++) {
        point.x = x;
        point.y = y;
        point.z = z;
        VSub(point, point, Wave_Sources[i]);
        VDot(length, point, point);
        if (length == 0.0) {
            length = 1.0;
        }

        length = sqrt(length);
        index = (length * texture->Frequency * frequency[i]) + texture->Phase;
        sinValue = cycloidal(index);

        scalar = sinValue * texture->Bump_Amount / frequency[i];
        VScale(point, point, scalar / length / (DBL)NUMBER_OF_WAVES);
        VAdd(*normal, *normal, point);
    }
    VNormalize(*normal, *normal);
}

void
bumps(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal)
{
    Vector3D bumpTurb;

    if (texture->Bump_Amount == 0.0) {
        return; /* why are we here?? */
    }

    if (Options & DEBUGGING) {
        printf("bumps %g %g %g\n", x, y, z);
    }

    DNoise(&bumpTurb, x, y, z); /* Get Normal Displacement Val. */
    VScale(bumpTurb, bumpTurb, texture->Bump_Amount);
    VAdd(*normal, *normal, bumpTurb); /* displace "normal" */
    VNormalize(*normal, *normal);     /* normalize normal! */
}

/*
dents is similar to bumps, but uses noise() to control the amount of
dnoise() perturbation of the object normal...
*/
void
dents(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal)
{
    Vector3D stuccoTurb;
    DBL noise;

    if (texture->Bump_Amount == 0.0) {
        return; /* why are we here?? */
    }

    noise = Noise(x, y, z);

    noise = noise * noise * noise * texture->Bump_Amount;

    if (Options & DEBUGGING) {
        printf("dents %g %g %g noise %g\n", x, y, z, noise);
    }

    DNoise(&stuccoTurb, x, y, z); /* Get Normal Displacement Val. */

    VScale(stuccoTurb, stuccoTurb, noise);
    VAdd(*normal, *normal, stuccoTurb); /* displace "normal" */
    VNormalize(*normal, *normal);       /* normalize normal! */
}

/*
    Ideas garnered from the April 89 Byte Graphics Supplement on RenderMan,
    refined from "The RenderMan Companion, by Steve Upstill of Pixar, (C) 1990
    Addison-Wesley.
*/

/*
    wrinkles - This is my implementation of the dented() routine, using
    a surface iterative fractal derived from DTurbulence.  This is a 3-D vers.
    (thanks to DNoise()...) of the usual version using the singular Noise()...
    Seems to look a lot like wrinkles, however... (hmmm)
*/

void
wrinkles(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal)
{
    register int i;
    register DBL scale = 1.0;
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
        DNoise(&value, x * scale, y * scale, z * scale); /* * scale,*/
        result.x += fabsInline(value.x / scale);
        result.y += fabsInline(value.y / scale);
        result.z += fabsInline(value.z / scale);
    }

    VScale(result, result, texture->Bump_Amount);
    VAdd(*normal, *normal, result); /* displace "normal" */
    VNormalize(*normal, *normal);   /* normalize normal! */
}
