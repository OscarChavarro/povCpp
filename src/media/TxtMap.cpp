/****************************************************************************
 *                     txtmap.c
 *
 *  This module implements the mapped textures including image map, bump map
 *  and material map.
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

#include "media/TxtMap.h"
#include "common/Frame.h"
#include "common/Matrices.h"
#include "common/PovProto.h"
#include "common/Vector.h"
#include "media/Texture.h"

extern int cylindricalImageMap(
    DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v);
extern int torusImageMap(DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v);
extern int sphericalImageMap(
    DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v);
extern int planarImageMap(
    DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v);
extern void noInterpolation(
    RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index);
extern DBL bilinear(DBL *corners, DBL x, DBL y);
extern DBL normDist(DBL *corners, DBL x, DBL y);
extern void interp(
    RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index);
extern void imageColourAt(
    RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index);

/*
    2-D to 3-D Procedural Texture Mapping of a Bitmapped Image onto an Object:

A. Simplistic (planar) method of image projection devised by DKB and AAC:

    1. Transform texture in 3-D space if requested.
    2. Determine local object 2-d coords from 3-d coords by <X Y Z> triple.
    3. Return pixel color value at that position on the 2-d plane of "Image".
    3. Map colour value in Image [0..255] to a more normal colour range [0..1].

B. Specialized shape projection variations by Alexander Enzmann:

    1. Cylindrical mapping
    2. Spherical mapping
    3. Torus mapping
*/

void
imageMap(DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour)
{
    /* determine local object 2-d coords from 3-d coords */
    /* "unwrap" object 2-d coord onto flat 2-d plane */
    /* return pixel color value at that posn on 2-d plane */

    DBL xcoor = 0.0, ycoor = 0.0;
    int regNumber;

    if (map(x, y, z, texture, texture->Image, &xcoor, &ycoor)) {
        makeColour(colour, 1.0, 1.0, 1.0);
        colour->Alpha = 1.0;
        return;
    }
    imageColourAt(texture->Image, xcoor, ycoor, colour, &regNumber);
}

/* Very different stuff than the other routines here. This routine takes  */
/* an intersection point and a texture and returns a new texture based on */
/* the index/color of that point in an image/materials map. CdW 7/91        */
Texture *
materialMap(Vector3D *intersectionPoint, Texture *texture)
{
    Vector3D transformedPoint;
    register DBL x;
    register DBL y;
    register DBL z;
    DBL xcoor = 0.0, ycoor = 0.0;
    int regNumber = 0;
    RGBAColor colour;
    int materialNumber = 0;
    Texture *tempTex;
    int numtex;

    makeColour(&colour, 0.0, 0.0, 0.0);
    colour.Alpha = 0.0;

    if (texture->Texture_Transformation) {
        MInverseTransformVector(&transformedPoint, intersectionPoint,
            texture->Texture_Transformation);
    } else {
        transformedPoint = *intersectionPoint;
    }

    x = transformedPoint.x;
    y = transformedPoint.y;
    z = transformedPoint.z;

    /* now we have transformed x, y, z we use image mapping routine */
    /* to determine texture index */
    if (map(x, y, z, texture, texture->Material_Image, &xcoor, &ycoor)) {
        materialNumber = 0;
    } else {
        imageColourAt(
            texture->Material_Image, xcoor, ycoor, &colour, &regNumber);

        if (texture->Material_Image->Colour_Map == nullptr) {
            materialNumber = (int)colour.Red * 255;
        } else {
            materialNumber = regNumber;
        }
    }

    /* Now I've got the Material number, I just have to find it in the */
    /* texture linked list and return it to Determine_Surface_Colour    */
    /* printf("-B-Num Mt#%d Mn#%d\n",texture->Number_Of_Materials,
     * Material_Number);    */
    if (materialNumber > texture->Number_Of_Materials) {
        materialNumber %= texture->Number_Of_Materials;
    }
    for (numtex = 0, tempTex = texture->Next_Material;
         tempTex->Next_Material != nullptr && numtex < materialNumber;
         tempTex = tempTex->Next_Material, numtex++) {
        ; /* do nothing */
    }

    /* texture = Temp_Tex;  */

    return (tempTex);
}

void
bumpMap(DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal)
{
    DBL xcoor = 0.0, ycoor = 0.0;
    int index;
    int index2;
    int index3;
    RGBAColor colour;
    RGBAColor colour2;
    RGBAColor colour3;
    Vector3D p1;
    Vector3D p2;
    Vector3D p3;
    Vector3D bumpNormal;
    Vector3D xprime;
    Vector3D yprime;
    Vector3D zprime;
    Vector3D temp;
    DBL length;
    makeColour(&colour, 0.0, 0.0, 0.0);
    colour.Alpha = 0.0;
    makeColour(&colour2, 0.0, 0.0, 0.0);
    colour2.Alpha = 0.0;
    makeColour(&colour3, 0.0, 0.0, 0.0);
    colour3.Alpha = 0.0;

    /* going to have to change this */
    /* need to know if bump point is off of image for all 3 points */

    if (map(x, y, z, texture, texture->Bump_Image, &xcoor, &ycoor)) {
        makeColour(&colour, 1.0, 1.0, 1.0);
        colour.Alpha = 1.0;
        index = 255;
        return;
    }
    imageColourAt(texture->Bump_Image, xcoor, ycoor, &colour, &index);

    xcoor--;
    ycoor++;
    if (xcoor < 0.0) {
        xcoor += (DBL)texture->Bump_Image->iwidth;
    } else if (xcoor >= texture->Bump_Image->iwidth) {
        xcoor -= (DBL)texture->Bump_Image->iwidth;
    }
    if (ycoor < 0.0) {
        ycoor += (DBL)texture->Bump_Image->iheight;
    } else if (ycoor >= (DBL)texture->Bump_Image->iheight) {
        ycoor -= (DBL)texture->Bump_Image->iheight;
    }
    imageColourAt(texture->Bump_Image, xcoor, ycoor, &colour2, &index2);

    xcoor += 2.0;
    if (xcoor < 0.0) {
        xcoor += (DBL)texture->Bump_Image->iwidth;
    } else if (xcoor >= texture->Bump_Image->iwidth) {
        xcoor -= (DBL)texture->Bump_Image->iwidth;
    }

    imageColourAt(texture->Bump_Image, xcoor, ycoor, &colour3, &index3);

    if (Options & DEBUGGING) {
        printf("Bump Map %g %g %g xcoor %f ycoor %f\n", x, y, z, xcoor, ycoor);
    }

    if (texture->Bump_Image->Colour_Map == nullptr ||
        texture->Bump_Image->Use_Colour_Flag) {
        p1.x = 0;
        p1.y =
            texture->Bump_Amount *
            (0.229 * colour.Red + 0.587 * colour.Green + 0.114 * colour.Blue);
        p1.z = 0;
        p2.x = 0;
        p2.y = texture->Bump_Amount *
               (0.229 * colour2.Red + 0.587 * colour2.Green +
                   0.114 * colour2.Blue);
        p2.z = 1;
        p3.x = 1;
        p3.y = texture->Bump_Amount *
               (0.229 * colour3.Red + 0.587 * colour3.Green +
                   0.114 * colour3.Blue);
        p3.z = 1;
    } else {
        p1.x = 0;
        p1.y = texture->Bump_Amount * index;
        p1.z = 0;
        p2.x = 0;
        p2.y = texture->Bump_Amount * index2;
        p2.z = 1;
        p3.x = 1;
        p3.y = texture->Bump_Amount * index3;
        p3.z = 1;
    }
    /* we have points 1,2,3 for a triangle now we need the surface normal for it
     */
    VSub(xprime, p1, p2);
    VSub(yprime, p3, p2);
    VCross(bumpNormal, yprime, xprime);
    VNormalize(bumpNormal, bumpNormal);

    makeVector(&yprime, normal->x, normal->y, normal->z);
    makeVector(&temp, 0.0, 1.0, 0.0);
    VCross(xprime, yprime, temp);
    VLength(length, xprime);
    if (length < 1.0e-9) {
        if (fabs(normal->y - 1.0) < Small_Tolerance) {
            makeVector(&yprime, 0.0, 1.0, 0.0);
            makeVector(&xprime, 1.0, 0.0, 0.0);
            length = 1.0;
        } else {
            makeVector(&yprime, 0.0, -1.0, 0.0);
            makeVector(&xprime, 1.0, 0.0, 0.0);
            length = 1.0;
        }
    }
    VScale(xprime, xprime, 1.0 / length);
    VCross(zprime, xprime, yprime);
    VNormalize(zprime, zprime);
    VScale(xprime, xprime, bumpNormal.x);
    VScale(yprime, yprime, bumpNormal.y);
    VScale(zprime, zprime, bumpNormal.z);
    VAdd(temp, xprime, yprime);
    VAdd(*normal, temp, zprime);
    VNormalize(*normal, *normal);
}

void
imageColourAt(
    RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index)
{
    switch (image->Interpolation_Type) {
    case NO_INTERPOLATION:
        noInterpolation(image, xcoor, ycoor, colour, index);
        break;
    default:
        interp(image, xcoor, ycoor, colour, index);
        break;
    }
}

/* Map a point (x, y, z) on a cylinder of radius 1, height 1, that has its
    axis of symmetry along the y-axis to the square [0,1]x[0,1]. */
int
cylindricalImageMap(DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v)
{
    DBL len, theta;

    if ((image->Once_Flag) && ((y < 0.0) || (y > 1.0))) {
        return 0;
    }
    *v = fmod(y * image->height, image->height);

    /* Make sure this vector is on the unit sphere. */
    len = sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        return 0;
    }
    x /= len;
    z /= len;

    /* Determine its angle from the point (1, 0, 0) in the x-z plane. */
    len = sqrt(x * x + z * z);
    if (len == 0.0) {
        return 0;
    }
    if (z == 0.0) {
        if (x > 0) {
            theta = 0.0;
        } else {
            theta = M_PI;
        }
    } else {
        theta = acos(x / len);
        if (z < 0.0) {
            theta = 2.0 * M_PI - theta;
        }
    }
    theta /= 2.0 * M_PI; /* This will be from 0 to 1 */

    *u = (theta * image->width);
    return 1;
}

/* Map a point (x, y, z) on a torus  to a 2-d image. */
int
torusImageMap(DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v)
{
    DBL len, phi, theta;
    DBL r0;

    r0 = image->Image_Gradient.x;

    /* Determine its angle from the x-axis. */
    len = sqrt(x * x + z * z);
    if (len == 0.0) {
        return 0;
    }
    if (z == 0.0) {
        if (x > 0) {
            theta = 0.0;
        } else {
            theta = M_PI;
        }
    } else {
        theta = acos(x / len);
        if (z < 0.0) {
            theta = 2.0 * M_PI - theta;
        }
    }

    theta = 0.0 - theta;

    /* Now rotate about the y-axis to get the point (x, y, z) into the x-y
     * plane. */
    x = len - r0;
    len = sqrt(x * x + y * y);
    phi = acos(-x / len);
    if (y > 0.0) {
        phi = 2.0 * M_PI - phi;
    }

    /* Determine the parametric coordinates. */
    theta /= 2.0 * M_PI;
    phi /= 2.0 * M_PI;
    *u = (-theta * image->width);
    *v = (phi * image->height);
    return 1;
}

/* Map a point (x, y, z) on a sphere of radius 1 to a 2-d image. (Or is it the
    other way around?) */
int
sphericalImageMap(DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v)
{
    DBL len, phi, theta;

    /* Make sure this vector is on the unit sphere. */
    len = sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        return 0;
    }
    x /= len;
    y /= len;
    z /= len;

    /* Determine its angle from the x-z plane. */
    phi = 0.5 + asin(y) / M_PI; /* This will be from 0 to 1 */

    /* Determine its angle from the point (1, 0, 0) in the x-z plane. */
    len = sqrt(x * x + z * z);
    if (len == 0.0) {
        /* This point is at one of the poles. Any value of xcoord will be
         * ok...*/
        theta = 0;
    } else {
        if (z == 0.0) {
            if (x > 0) {
                theta = 0.0;
            } else {
                theta = M_PI;
            }
        } else {
            theta = acos(x / len);
            if (z < 0.0) {
                theta = 2.0 * M_PI - theta;
            }
        }
        theta /= 2.0 * M_PI; /* This will be from 0 to 1 */
    }
    *u = (theta * image->width);
    *v = (phi * image->height);
    return 1;
}

/*
    2-D to 3-D Procedural Texture Mapping of a Bitmapped Image onto an Object:

    Simplistic planar method of object image projection devised by DKB and AAC.

    1. Transform texture in 3-D space if requested.
    2. Determine local object 2-d coords from 3-d coords by <X Y Z> triple.
    3. Return pixel color value at that position on the 2-d plane of "Image".
    3. Map colour value in Image [0..255] to a more normal colour range [0..1].
*/

/* Return 0 if there is no color at this point (i.e. invisible), return 1
    if a good mapping is found. */
int
planarImageMap(DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v)
{
    if (image->Image_Gradient.x != 0.0) {
        if ((image->Once_Flag) && ((x < 0.0) || (x > 1.0))) {
            return 0;
        }
        if (image->Image_Gradient.x > 0) {
            *u = fmod(x * image->width, image->width);
        } else {
            *v = fmod(x * image->height, image->height);
        }
    }
    if (image->Image_Gradient.y != 0.0) {
        if ((image->Once_Flag) && ((y < 0.0) || (y > 1.0))) {
            return 0;
        }
        if (image->Image_Gradient.y > 0) {
            *u = fmod(y * image->width, image->width);
        } else {
            *v = fmod(y * image->height, image->height);
        }
    }
    if (image->Image_Gradient.z != 0.0) {
        if ((image->Once_Flag) && ((z < 0.0) || (z > 1.0))) {
            return 0;
        }
        if (image->Image_Gradient.z > 0) {
            *u = fmod(z * image->width, image->width);
        } else {
            *v = fmod(z * image->height, image->height);
        }
    }
    return 1;
}

/* Map returns 1 if no color found (invisible) or 0 if color found */
int
map(DBL x, DBL y, DBL z, Texture *texture, RGBAImage *image, DBL *xcoor,
    DBL *ycoor)
{
    /* determine local object 2-d coords from 3-d coords */
    /* "unwrap" object 2-d coord onto flat 2-d plane */
    /* return pixel color value at that posn on 2-d plane */

    /* This causes problems so let's do without it for this release
    if ((turb = texture->Turbulence) != 0.0)
    {
        DTurbulence (&textureTurbulence, x, y, z, texture->Octaves);
        x += TextureTurbulence.x * turb;
        y += TextureTurbulence.y * turb;
        z += TextureTurbulence.z * turb;
    }
    */

    /* Now determine which mapper to use. */
    switch (image->Map_Type) {
    case PLANAR_MAP:
        if (!planarImageMap(x, y, z, image, xcoor, ycoor)) {
            return (1);
        }
        break;
    case SPHERICAL_MAP:
        if (!sphericalImageMap(x, y, z, image, xcoor, ycoor)) {
            return (1);
        }
        break;
    case CYLINDRICAL_MAP:
        if (!cylindricalImageMap(x, y, z, image, xcoor, ycoor)) {
            return (1);
        }
        break;
    case TORUS_MAP:
        if (!torusImageMap(x, y, z, image, xcoor, ycoor)) {
            return (1);
        }
        break;
    default:
        if (!planarImageMap(x, y, z, image, xcoor, ycoor)) {
            return (1);
        }
        break;
    }
    /* Now make sure the point is on the image */
    *ycoor += Small_Tolerance;
    *xcoor += Small_Tolerance;
    /* Compensate for y coordinates on the images being upsidedown */
    *ycoor = (DBL)image->iheight - *ycoor;

    if (*xcoor < 0.0) {
        *xcoor += (DBL)image->iwidth;
    } else if (*xcoor >= (DBL)image->iwidth) {
        *xcoor -= (DBL)image->iwidth;
    }

    if (*ycoor < 0.0) {
        *ycoor += (DBL)image->iheight;
    } else if (*ycoor >= (DBL)image->iheight) {
        *ycoor -= (DBL)image->iheight;
    }

    if (Options & DEBUGGING) {
        printf("\nmap %g %g %g xcoor %f ycoor %f ih %d iw %d\n", x, y, z,
            *xcoor, *ycoor, image->iheight, image->iwidth);
    }

    if ((*xcoor >= (DBL)image->iwidth) || (*ycoor >= (DBL)image->iheight) ||
        (*xcoor < 0.0) || (*ycoor < 0.0)) {
        printf("\nPicture index out of range\n");
        closeAll();
        exit(1);
    }

    return (0);
}

void
noInterpolation(
    RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index)
{
    ImageLine *line;
    int iycoor;
    int ixcoor;
    RGBAPixel *mapColour;

    if (xcoor < 0.0) {
        xcoor += (DBL)image->iwidth;
    } else if (xcoor >= (DBL)image->iwidth) {
        xcoor -= (DBL)image->iwidth;
    }
    if (ycoor < 0.0) {
        ycoor += (DBL)image->iheight;
    } else if (ycoor >= (DBL)image->iheight) {
        ycoor -= (DBL)image->iheight;
    }

    iycoor = (int)ycoor;
    ixcoor = (int)xcoor;
    if (image->Colour_Map == nullptr) {
        line = &image->data.rgb_lines[iycoor];
        colour->Red += (DBL)line->red[ixcoor] / 255.0;
        colour->Green += (DBL)line->green[ixcoor] / 255.0;
        colour->Blue += (DBL)line->blue[ixcoor] / 255.0;
        *index = -1;
    } else {
        *index = image->data.map_lines[iycoor][ixcoor];
        mapColour = &image->Colour_Map[*index];
        /*printf ("icat index %d xc %d yc %d  CLR %d %d %d
     %d\n",*index,ixcoor,iycoor,
     map_colour->Red,map_colour->Green,map_colour->Blue,map_colour->Alpha ); */
        colour->Red += (DBL)mapColour->Red / 255.0;
        colour->Green += (DBL)mapColour->Green / 255.0;
        colour->Blue += (DBL)mapColour->Blue / 255.0;
        colour->Alpha += (DBL)mapColour->Alpha / 255.0;
    }

    if (Options & DEBUGGING) {
        printf("\n no_interpolation index %d xc %d yc %d \n", *index, ixcoor,
            iycoor);
    }
}

/* Interpolate color and alpha values when mapping */
void
interp(RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index)
{
    int iycoor;
    int ixcoor;
    int i;
    int cornersIndex[4];
    DBL indexCrn[4];
    RGBAColor cornerColour[4];
    DBL redCrn[4];
    DBL greenCrn[4];
    DBL blueCrn[4];
    DBL alphaCrn[4];
    DBL val1 = 0, val2 = 0, val3 = 0, val4 = 0;

    iycoor = (int)ycoor;
    ixcoor = (int)xcoor;
    for (i = 0; i < 4; i++) {
        makeColour(&cornerColour[i], 0.0, 0.0, 0.0);
        cornerColour[i].Alpha = 0.0;
    }
    /* OK, now that you have the corners, what are you going to do with them? */
    if (image->Interpolation_Type == BILINEAR) {
        noInterpolation(image, (DBL)ixcoor + 1, (DBL)iycoor, &cornerColour[0],
            &cornersIndex[0]);
        noInterpolation(image, (DBL)ixcoor, (DBL)iycoor, &cornerColour[1],
            &cornersIndex[1]);
        noInterpolation(image, (DBL)ixcoor + 1, (DBL)iycoor - 1,
            &cornerColour[2], &cornersIndex[2]);
        noInterpolation(image, (DBL)ixcoor, (DBL)iycoor - 1, &cornerColour[3],
            &cornersIndex[3]);
        for (i = 0; i < 4; i++) {
            redCrn[i] = cornerColour[i].Red;
            greenCrn[i] = cornerColour[i].Green;
            blueCrn[i] = cornerColour[i].Blue;
            alphaCrn[i] = cornerColour[i].Alpha;
            /* printf("Crn %d = %lf %lf
             * %lf\n",i,Red_Crn[i],Blue_Crn[i],Green_Crn[i]); */
        }

        val1 = bilinear(redCrn, xcoor, ycoor);
        val2 = bilinear(greenCrn, xcoor, ycoor);
        val3 = bilinear(blueCrn, xcoor, ycoor);
        val4 = bilinear(alphaCrn, xcoor, ycoor);
    }
    if (image->Interpolation_Type == NORMALIZED_DIST) {
        noInterpolation(image, (DBL)ixcoor, (DBL)iycoor - 1, &cornerColour[0],
            &cornersIndex[0]);
        noInterpolation(image, (DBL)ixcoor + 1, (DBL)iycoor - 1,
            &cornerColour[1], &cornersIndex[1]);
        noInterpolation(image, (DBL)ixcoor, (DBL)iycoor, &cornerColour[2],
            &cornersIndex[2]);
        noInterpolation(image, (DBL)ixcoor + 1, (DBL)iycoor, &cornerColour[3],
            &cornersIndex[3]);
        for (i = 0; i < 4; i++) {
            redCrn[i] = cornerColour[i].Red;
            greenCrn[i] = cornerColour[i].Green;
            blueCrn[i] = cornerColour[i].Blue;
            alphaCrn[i] = cornerColour[i].Alpha;
            /* printf("Crn %d = %lf %lf
             * %lf\n",i,Red_Crn[i],Blue_Crn[i],Green_Crn[i]); */
        }

        val1 = normDist(redCrn, xcoor, ycoor);
        val2 = normDist(greenCrn, xcoor, ycoor);
        val3 = normDist(blueCrn, xcoor, ycoor);
        val4 = normDist(alphaCrn, xcoor, ycoor);
    }

    colour->Red += val1;
    colour->Green += val2;
    colour->Blue += val3;
    colour->Alpha += val4;
    /* printf("Final = %lf %lf %lf\n",val1,val2,val3);  */
    /* use bilinear for index try average later */
    for (i = 0; i < 4; i++) {
        indexCrn[i] = (DBL)cornersIndex[i];
    }
    if (image->Interpolation_Type == BILINEAR) {
        *index = (int)(bilinear(indexCrn, xcoor, ycoor) + 0.5);
    }
    if (image->Interpolation_Type == NORMALIZED_DIST) {
        *index = (int)(normDist(indexCrn, xcoor, ycoor) + 0.5);
    }
}

/* These interpolation techniques are taken from an article by */
/* Girish T. Hagan in the C Programmer's Journal V 9 No. 8 */
/* They were adapted for POV-Ray by CdW */
DBL
bilinear(DBL *corners, DBL x, DBL y)
{
    DBL p, q;
    DBL val = 0.0;

    p = x - (int)x;
    q = y - (int)y;
    if ((p == 0.0) && (q == 0.0)) {
        return (*corners); /* upper left */
    }

    val = (p * q * *corners) + (q * (1 - p) * *(corners + 1)) +
          (p * (1 - q) * *(corners + 2)) + ((1 - p) * (1 - q) * *(corners + 3));
    return (val);
}

#define MAX_PTS 4
#define PYTHAGOREAN_SQ(a, b) ((a) * (a) + (b) * (b))

DBL
normDist(DBL *corners, DBL x, DBL y)
{
    register int i;

    DBL p, q;
    DBL wts[MAX_PTS];
    DBL sumInvWts = 0.0;
    DBL sumI = 0.0;

    p = x - (int)x;
    q = y - (int)y;

    if ((p == 0.0) && (q == 0.0)) {
        return (*corners); /* upper left */
    }

    wts[0] = PYTHAGOREAN_SQ(p, q);
    wts[1] = PYTHAGOREAN_SQ(1 - p, q);
    wts[2] = PYTHAGOREAN_SQ(p, 1 - q);
    wts[3] = PYTHAGOREAN_SQ(1 - p, 1 - q);

    for (i = 0; i < MAX_PTS; i++) {
        sumInvWts += 1 / wts[i];
        sumI += *(corners + i) / wts[i];
    }

    return (sumI / sumInvWts);
}
