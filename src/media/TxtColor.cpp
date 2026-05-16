/****************************************************************************
 *                     txtcolor.c
 *
 *  This module implements solid texturing functions that modify the color
 *  transparency of an object's surface.
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
    Further Ideas Garnered from "The RenderMan Companion" (Addison Wesley).
*/

#include "media/TxtColor.h"
#include "common/Frame.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "media/Texture.h"
#include "media/TxtMap.h"
#include "media/TxtTest.h"

static constexpr double COORDINATE_LIMIT = 1.0e17;

void
colourAt(RGBAColor *colour, Texture *texture, Vector3D *intersectionPoint)
{
    register DBL x;
    register DBL y;
    register DBL z;
    Vector3D transformedPoint;

    if ((intersectionPoint->x > COORDINATE_LIMIT) ||
        (intersectionPoint->y > COORDINATE_LIMIT) ||
        (intersectionPoint->z > COORDINATE_LIMIT) ||
        (intersectionPoint->x < -COORDINATE_LIMIT) ||
        (intersectionPoint->y < -COORDINATE_LIMIT) ||
        (intersectionPoint->z < -COORDINATE_LIMIT)) {
        makeVector(&transformedPoint, 0.0, 0.0, 0.0);
    } else {
        if (texture->Texture_Transformation) {
            MInverseTransformVector(&transformedPoint, intersectionPoint,
                texture->Texture_Transformation);
        } else {
            transformedPoint = *intersectionPoint;
        }
    }

    x = transformedPoint.x;
    y = transformedPoint.y;
    z = transformedPoint.z;

    switch (texture->Texture_Number) {
    case NO_TEXTURE:
        /* No colouring texture has been specified - make it black. */
        makeColour(colour, 0.0, 0.0, 0.0);
        colour->Alpha = 0.0;
        break;

    case COLOUR_TEXTURE:
        colour->Red += texture->Colour1->Red;
        colour->Green += texture->Colour1->Green;
        colour->Blue += texture->Colour1->Blue;
        colour->Alpha += texture->Colour1->Alpha;
        break;

    case BOZO_TEXTURE:
        bozo(x, y, z, texture, colour);
        break;

    case MARBLE_TEXTURE:
        marble(x, y, z, texture, colour);
        break;

    case WOOD_TEXTURE:
        wood(x, y, z, texture, colour);
        break;

    case BRICK_TEXTURE:
        brick(x, y, z, texture, colour);
        break;

    case CHECKER_TEXTURE:
        checker(x, y, z, texture, colour);
        break;

    case CHECKER_TEXTURE_TEXTURE:
        checkerTexture(x, y, z, texture, colour);
        break;

    case SPOTTED_TEXTURE:
        spotted(x, y, z, texture, colour);
        break;

    case AGATE_TEXTURE:
        agate(x, y, z, texture, colour);
        break;

    case GRANITE_TEXTURE:
        granite(x, y, z, texture, colour);
        break;

    case GRADIENT_TEXTURE:
        gradient(x, y, z, texture, colour);
        break;

    case IMAGEMAP_TEXTURE:
        imageMap(x, y, z, texture, colour);
        break;

    case ONION_TEXTURE:
        onion(x, y, z, texture, colour);
        break;

    case LEOPARD_TEXTURE:
        leopard(x, y, z, texture, colour);
        break;

    case PAINTED1_TEXTURE:
        painted1(x, y, z, texture, colour);
        break;

    case PAINTED2_TEXTURE:
        painted2(x, y, z, texture, colour);
        break;

    case PAINTED3_TEXTURE:
        painted3(x, y, z, texture, colour);
        break;
    }
}

void
agate(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    register DBL noise;
    register DBL hue;
    RGBAColor newColour;

    noise =
        cycloidal(1.3 * Turbulence(x, y, z, texture->Octaves) + 1.1 * z) + 1;
    noise *= 0.5;
    noise = pow(noise, 0.77);

    if (Options & DEBUGGING) {
        printf("agate %g %g %g noise %g\n", x, y, z, noise);
    }

    if (texture->Colour_Map != nullptr) {
        computeColour(&newColour, texture->Colour_Map, noise);
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

void
bozo(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    register DBL noise;
    register DBL turb;
    RGBAColor newColour;
    Vector3D bozoTurbulence;

    if (Options & DEBUGGING) {
        printf("bozo %g %g %g ", x, y, z);
    }

    if ((turb = texture->Turbulence) != 0.0) {
        DTurbulence(&bozoTurbulence, x, y, z, texture->Octaves);
        x += bozoTurbulence.x * turb;
        y += bozoTurbulence.y * turb;
        z += bozoTurbulence.z * turb;
    }

    noise = Noise(x, y, z);

    if (Options & DEBUGGING) {
        printf("noise %g\n", noise);
    }

    if (texture->Colour_Map != nullptr) {
        computeColour(&newColour, texture->Colour_Map, noise);
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
brick(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    DBL xr, yr, zr;

    xr = fabs(fmod(x, 1.0));
    yr = fabs(fmod(y, 1.0));
    zr = fabs(fmod(z, 1.0));

    *colour = *texture->Colour2;

    if (Options & DEBUGGING) {
        printf("brick %g %g %g\n", x, y, z);
    }

    if (xr > 0 && xr < texture->Mortar) {
        *colour = *texture->Colour1;
        return;
    }
    if (yr > 0 && yr < texture->Mortar) {
        *colour = *texture->Colour1;
        return;
    }
    if (zr > 0 && zr < texture->Mortar) {
        *colour = *texture->Colour1;
    }
}

void
checker(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    int brkindx;

    x += Small_Tolerance; /* add a small offset to x, y, z, axes to prevent
                             noise */
    y += Small_Tolerance;
    z += Small_Tolerance;

    /* AAC: was just x + z */
    /* AAC: Small_Tolerance added to get around Microsoft C (int) bug */
    brkindx = (int)(FLOOR(x) + FLOOR(y) + FLOOR(z));

    if (Options & DEBUGGING) {
        printf("checker %g %g %g\n", x, y, z);
    }

    if (brkindx & 1) {
        *colour = *texture->Colour1;
    } else {
        *colour = *texture->Colour2;
    }
}

void
checkerTexture(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    int brkindx;
    Vector3D point;

    x += Small_Tolerance; /* add a small offset to x, y, z, axes to prevent
                             noise */
    y += Small_Tolerance;
    z += Small_Tolerance;

    brkindx = (int)(FLOOR(x) + FLOOR(y) + FLOOR(z));

    if (Options & DEBUGGING) {
        printf("checker_texture %g %g %g\n", x, y, z);
    }

    makeVector(&point, x, y, z);

    if (brkindx & 1) {
        colourAt(colour, ((Texture *)texture->Colour1), &point);
    } else {
        colourAt(colour, ((Texture *)texture->Colour2), &point);
    }
}

/*
    Color Gradient Texture - gradient based on the fractional values of x, y or
    z, based on whether or not the given directional vector is a 1.0 or a 0.0.
    Note - ONLY works with colour maps, preferably one that is circular - i.e.
    the last defined colour (value 1.001) is the same as the first colour (with
    a value of 0.0) in the map.  The basic concept of this is from DBW Render,
    but Dave Wecker's only supports simple Y axis gradients.
*/
void
gradient(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    RGBAColor newColour;
    DBL value = 0.0, turb;
    Vector3D gradTurbulence;

    if ((turb = texture->Turbulence) != 0.0) {
        DTurbulence(&gradTurbulence, x, y, z, texture->Octaves);
        x += gradTurbulence.x * turb;
        y += gradTurbulence.y * turb;
        z += gradTurbulence.z * turb;
    }

    if (texture->Colour_Map == nullptr) {
        return;
    }
    if (texture->Texture_Gradient.x != 0.0) {
        x = FABS(x);
        value += x - FLOOR(x); /* obtain fractional X component */
    }
    if (texture->Texture_Gradient.y != 0.0) {
        y = FABS(y);
        value += y - FLOOR(y); /* obtain fractional Y component */
    }
    if (texture->Texture_Gradient.z != 0.0) {
        z = FABS(z);
        value += z - FLOOR(z); /* obtain fractional Z component */
    }
    value = ((value > 1.0) ? fmod(value, 1.0) : value); /* clamp to 1.0 */

    if (Options & DEBUGGING) {
        printf("gradient %g %g %g value %g\n", x, y, z, value);
    }

    computeColour(&newColour, texture->Colour_Map, value);
    colour->Red += newColour.Red;
    colour->Green += newColour.Green;
    colour->Blue += newColour.Blue;
    colour->Alpha += newColour.Alpha;
}

/*
    Granite - kind of a union of the "spotted" and the "dented" textures,
    using a 1/f fractal noise function for color values.  Typically used
    w/ small scaling values.  Should work with colour maps for pink granite...
*/
void
granite(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    register int i;
    register DBL temp;
    register DBL noise = 0.0;
    register DBL freq = 1.0;
    RGBAColor newColour;

    for (i = 0; i < 6; freq *= 2.0, i++) {
        temp = 0.5 - Noise(x * 4 * freq, y * 4 * freq, z * 4 * freq);
        temp = FABS(temp);
        noise += temp / freq;
    }

    if (Options & DEBUGGING) {
        printf("granite %g %g %g noise %g\n", x, y, z, noise);
    }

    if (texture->Colour_Map != nullptr) {
        computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += noise; /* "white (1.0) * noise" */
    colour->Green += noise;
    colour->Blue += noise;
}

void
marble(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    register DBL noise;
    register DBL hue;
    RGBAColor newColour;

    noise = triangleWave(
        x + Turbulence(x, y, z, texture->Octaves) * texture->Turbulence);

    if (Options & DEBUGGING) {
        printf("marble %g %g %g noise %g \n", x, y, z, noise);
    }

    if (texture->Colour_Map != nullptr) {
        computeColour(&newColour, texture->Colour_Map, noise);
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

/*
    With a little reflectivity and brilliance, can look like organ pipe
    metal.    With tiny scaling values can look like masonry or concrete.
    Works with color maps.
*/
void
spotted(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    register DBL noise;
    RGBAColor newColour;

    noise = Noise(x, y, z);

    if (Options & DEBUGGING) {
        printf("spotted %g %g %g\n", x, y, z);
    }

    if (texture->Colour_Map != nullptr) {
        computeColour(&newColour, texture->Colour_Map, noise);
        colour->Red += newColour.Red;
        colour->Green += newColour.Green;
        colour->Blue += newColour.Blue;
        colour->Alpha += newColour.Alpha;
        return;
    }

    colour->Red += noise; /* "white (1.0) * noise" */
    colour->Green += noise;
    colour->Blue += noise;
}

void
wood(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    register DBL noise;
    register DBL length;
    Vector3D woodTurbulence;
    Vector3D point;
    RGBAColor newColour;

    DTurbulence(&woodTurbulence, x, y, z, texture->Octaves);

    if (Options & DEBUGGING) {
        printf("wood %g %g %g", x, y, z);
    }

    point.x = cycloidal((x + woodTurbulence.x) * texture->Turbulence);
    point.y = cycloidal((y + woodTurbulence.y) * texture->Turbulence);
    point.z = 0.0;

    point.x += x;
    point.y += y;

    /*  point.z += z;         Deleted per David Buck --  BP 7/91 */

    VLength(length, point);

    noise = triangleWave(length);

    if (Options & DEBUGGING) {
        printf("noise %g\n", noise);
    }

    if (texture->Colour_Map != nullptr) {
        computeColour(&newColour, texture->Colour_Map, noise);
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

/* Two new textures by Scott Taylor LEOPARD & ONION */
/* SWT 7/18/91 */
void
leopard(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    /* The variable noise is not used as noise in this function */
    register DBL noise;
    register DBL turb;
    register DBL temp1;
    register DBL temp2;
    register DBL temp3;
    RGBAColor newColour;
    Vector3D leopardTurbulence;

    if (Options & DEBUGGING) {
        printf("leopard %g %g %g ", x, y, z);
    }

    if ((turb = texture->Turbulence) != 0.0) {
        DTurbulence(&leopardTurbulence, x, y, z, texture->Octaves);
        x += leopardTurbulence.x * turb;
        y += leopardTurbulence.y * turb;
        z += leopardTurbulence.z * turb;
    }
    /* This form didn't work with Zortech 386 compiler */
    /* noise = Sqr((sin(x)+sin(y)+sin(z))/3); */
    /* So we break it down. */
    temp1 = sin(x);
    temp2 = sin(y);
    temp3 = sin(z);
    noise = Sqr((temp1 + temp2 + temp3) / 3);

    if (Options & DEBUGGING) {
        printf("temp123 %g %g %g  ", temp1, temp2, temp3);
    }

    if (Options & DEBUGGING) {
        printf("noise %g\n", noise);
    }

    if (texture->Colour_Map != nullptr) {
        computeColour(&newColour, texture->Colour_Map, noise);
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

/* SWT 7/18/91 */
void
onion(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    /* The variable noise is not used as noise in this function */
    register DBL noise;
    register DBL turb;
    RGBAColor newColour;
    Vector3D onionTurbulence;

    if (Options & DEBUGGING) {
        printf("onion %g %g %g ", x, y, z);
    }

    if ((turb = texture->Turbulence) != 0.0) {
        DTurbulence(&onionTurbulence, x, y, z, texture->Octaves);
        x += onionTurbulence.x * turb;
        y += onionTurbulence.y * turb;
        z += onionTurbulence.z * turb;
    }

    /* This ramp goes 0-1,1-0,0-1,1-0...
    noise = (fmod(std::sqrt(Sqr(x)+Sqr(y)+Sqr(z)),2.0)-1.0);
    if (noise<0.0) {noise = 0.0-noise;}
    */

    /* This ramp goes 0-1,0-1,0-1,0-1... */
    noise = (fmod(std::sqrt(Sqr(x) + Sqr(y) + Sqr(z)), 1.0));

    if (Options & DEBUGGING) {
        printf("noise %g\n", noise);
    }

    if (texture->Colour_Map != nullptr) {
        computeColour(&newColour, texture->Colour_Map, noise);
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
