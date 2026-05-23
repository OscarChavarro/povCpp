/****************************************************************************
 *                     txtmap.c
 *
 *  This module implements the mapped textures including image map, bump map
 *  and material map.
 *
 *****************************************************************************/

#include "media/MapTextureFixture.h"
#include "common/logger/Logger.h"
#include "common/LegacyBoolean.h"
#include <cstdio>
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"

extern int cylindricalImageMap(
    double x, double y, double z, RGBAImage *image, double *u, double *v);
extern int sphericalImageMap(
    double x, double y, double z, RGBAImage *image, double *u, double *v);
extern int planarImageMap(
    double x, double y, double z, RGBAImage *image, double *u, double *v);
extern void noInterpolation(RGBAImage *image, double xcoor, double ycoor,
    RGBAColor *colour, int *index);
extern void interp(RGBAImage *image, double xcoor, double ycoor,
    RGBAColor *colour, int *index);
extern void imageColourAt(RGBAImage *image, double xcoor, double ycoor,
    RGBAColor *colour, int *index);

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
MapTextureFixture::imageMap(
    double x, double y, double z, Texture *texture, RGBAColor *colour,
    int debugEnabled, double smallTolerance)
{
    /* determine local object 2-d coords from 3-d coords */
    /* "unwrap" object 2-d coord onto flat 2-d plane */
    /* return pixel color value at that posn on 2-d plane */

    double xcoor = 0.0, ycoor = 0.0;
    int regNumber;

    if (map(x, y, z, texture, texture->Image, &xcoor, &ycoor, smallTolerance)) {
        Color::makeColor(colour, 1.0, 1.0, 1.0);
        colour->Alpha = 1.0;
        return;
    }
    imageColourAt(texture->Image, xcoor, ycoor, colour, &regNumber);
}

/* Very different stuff than the other routines here. This routine takes  */
/* an intersection point and a texture and returns a new texture based on */
/* the index/color of that point in an image/materials map. CdW 7/91        */
Texture *
MapTextureFixture::materialMap(Vector3Dd *intersectionPoint, Texture *texture,
    int debugEnabled, double smallTolerance)
{
    Vector3Dd transformedPoint;
    double x;
    double y;
    double z;
    double xcoor = 0.0, ycoor = 0.0;
    int regNumber = 0;
    RGBAColor colour;
    int materialNumber = 0;
    Texture *tempTex;
    int numtex;

    Color::makeColor(&colour, 0.0, 0.0, 0.0);
    colour.Alpha = 0.0;

    if (texture->Texture_Transformation) {
        Transformation::MInverseTransformVector(&transformedPoint,
            intersectionPoint, texture->Texture_Transformation);
    } else {
        transformedPoint = *intersectionPoint;
    }

    x = transformedPoint.x;
    y = transformedPoint.y;
    z = transformedPoint.z;

    /* now we have transformed x, y, z we use image mapping routine */
    /* to determine texture index */
    if (map(x, y, z, texture, texture->Material_Image, &xcoor, &ycoor,
            smallTolerance)) {
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
    /* Logger::info("-B-Num Mt#%d Mn#%d\n",texture->Number_Of_Materials,
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
MapTextureFixture::bumpMap(
    double x, double y, double z, Texture *texture, Vector3Dd *normal,
    int debugEnabled, double smallTolerance)
{
    double xcoor = 0.0, ycoor = 0.0;
    int index;
    int index2;
    int index3;
    RGBAColor colour;
    RGBAColor colour2;
    RGBAColor colour3;
    Vector3Dd p1;
    Vector3Dd p2;
    Vector3Dd p3;
    Vector3Dd bumpNormal;
    Vector3Dd xprime;
    Vector3Dd yprime;
    Vector3Dd zprime;
    Vector3Dd temp;
    double length;
    Color::makeColor(&colour, 0.0, 0.0, 0.0);
    colour.Alpha = 0.0;
    Color::makeColor(&colour2, 0.0, 0.0, 0.0);
    colour2.Alpha = 0.0;
    Color::makeColor(&colour3, 0.0, 0.0, 0.0);
    colour3.Alpha = 0.0;

    /* going to have to change this */
    /* need to know if bump point is off of image for all 3 points */

    if (map(
            x, y, z, texture, texture->Bump_Image, &xcoor, &ycoor, smallTolerance)) {
        Color::makeColor(&colour, 1.0, 1.0, 1.0);
        colour.Alpha = 1.0;
        index = 255;
        return;
    }
    imageColourAt(texture->Bump_Image, xcoor, ycoor, &colour, &index);

    xcoor--;
    ycoor++;
    if (xcoor < 0.0) {
        xcoor += (double)texture->Bump_Image->iwidth;
    } else if (xcoor >= texture->Bump_Image->iwidth) {
        xcoor -= (double)texture->Bump_Image->iwidth;
    }
    if (ycoor < 0.0) {
        ycoor += (double)texture->Bump_Image->iheight;
    } else if (ycoor >= (double)texture->Bump_Image->iheight) {
        ycoor -= (double)texture->Bump_Image->iheight;
    }
    imageColourAt(texture->Bump_Image, xcoor, ycoor, &colour2, &index2);

    xcoor += 2.0;
    if (xcoor < 0.0) {
        xcoor += (double)texture->Bump_Image->iwidth;
    } else if (xcoor >= texture->Bump_Image->iwidth) {
        xcoor -= (double)texture->Bump_Image->iwidth;
    }

    imageColourAt(texture->Bump_Image, xcoor, ycoor, &colour3, &index3);

    if (debugEnabled) {
        Logger::info("Bump Map %g %g %g xcoor %f ycoor %f\n", x, y, z, xcoor, ycoor);
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
    VectorOps::vSub(xprime, p1, p2);
    VectorOps::vSub(yprime, p3, p2);
    bumpNormal = yprime.crossProduct(xprime);
    bumpNormal.normalize();

    *&yprime = Vector3Dd(normal->x, normal->y, normal->z);
    *&temp = Vector3Dd(0.0, 1.0, 0.0);
    xprime = yprime.crossProduct(temp);
    length = xprime.length();
    if (length < 1.0e-9) {
        if (fabs(normal->y - 1.0) < smallTolerance) {
            *&yprime = Vector3Dd(0.0, 1.0, 0.0);
            *&xprime = Vector3Dd(1.0, 0.0, 0.0);
            length = 1.0;
        } else {
            *&yprime = Vector3Dd(0.0, -1.0, 0.0);
            *&xprime = Vector3Dd(1.0, 0.0, 0.0);
            length = 1.0;
        }
    }
    xprime.scale(1.0 / length);
    zprime = xprime.crossProduct(yprime);
    zprime.normalize();
    xprime.scale(bumpNormal.x);
    yprime.scale(bumpNormal.y);
    zprime.scale(bumpNormal.z);
    VectorOps::vAdd(temp, xprime, yprime);
    VectorOps::vAdd(*normal, temp, zprime);
    (*normal).normalize();
}

void
MapTextureFixture::imageColourAt(
    RGBAImage *image, double xcoor, double ycoor, RGBAColor *colour, int *index)
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
MapTextureFixture::cylindricalImageMap(
    double x, double y, double z, RGBAImage *image, double *u, double *v)
{
    double len, theta;

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
MapTextureFixture::torusImageMap(
    double x, double y, double z, RGBAImage *image, double *u, double *v)
{
    double len, phi, theta;
    double r0;

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
MapTextureFixture::sphericalImageMap(
    double x, double y, double z, RGBAImage *image, double *u, double *v)
{
    double len, phi, theta;

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
MapTextureFixture::planarImageMap(
    double x, double y, double z, RGBAImage *image, double *u, double *v)
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
MapTextureFixture::map(double x, double y, double z, Texture *texture,
    RGBAImage *image, double *xcoor, double *ycoor, double smallTolerance)
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
    *ycoor += smallTolerance;
    *xcoor += smallTolerance;
    /* Compensate for y coordinates on the images being upsidedown */
    *ycoor = (double)image->iheight - *ycoor;

    if (*xcoor < 0.0) {
        *xcoor += (double)image->iwidth;
    } else if (*xcoor >= (double)image->iwidth) {
        *xcoor -= (double)image->iwidth;
    }

    if (*ycoor < 0.0) {
        *ycoor += (double)image->iheight;
    } else if (*ycoor >= (double)image->iheight) {
        *ycoor -= (double)image->iheight;
    }

    if ((*xcoor >= (double)image->iwidth) ||
        (*ycoor >= (double)image->iheight) || (*xcoor < 0.0) ||
        (*ycoor < 0.0)) {
        Logger::info("\nPicture index out of range\n");
        exit(1);
    }

    return (0);
}

void
MapTextureFixture::noInterpolation(
    RGBAImage *image, double xcoor, double ycoor, RGBAColor *colour, int *index)
{
    ImageLine *line;
    int iycoor;
    int ixcoor;
    RGBAPixel *mapColour;

    if (xcoor < 0.0) {
        xcoor += (double)image->iwidth;
    } else if (xcoor >= (double)image->iwidth) {
        xcoor -= (double)image->iwidth;
    }
    if (ycoor < 0.0) {
        ycoor += (double)image->iheight;
    } else if (ycoor >= (double)image->iheight) {
        ycoor -= (double)image->iheight;
    }

    iycoor = (int)ycoor;
    ixcoor = (int)xcoor;
    if (image->Colour_Map == nullptr) {
        line = &image->data.rgb_lines[iycoor];
        colour->Red += (double)line->red[ixcoor] / 255.0;
        colour->Green += (double)line->green[ixcoor] / 255.0;
        colour->Blue += (double)line->blue[ixcoor] / 255.0;
        *index = -1;
    } else {
        *index = image->data.map_lines[iycoor][ixcoor];
        mapColour = &image->Colour_Map[*index];
        /*Logger::info("icat index %d xc %d yc %d  CLR %d %d %d
     %d\n",*index,ixcoor,iycoor,
     map_colour->Red,map_colour->Green,map_colour->Blue,map_colour->Alpha ); */
        colour->Red += (double)mapColour->Red / 255.0;
        colour->Green += (double)mapColour->Green / 255.0;
        colour->Blue += (double)mapColour->Blue / 255.0;
        colour->Alpha += (double)mapColour->Alpha / 255.0;
    }

}

/* Interpolate color and alpha values when mapping */
void
MapTextureFixture::interp(
    RGBAImage *image, double xcoor, double ycoor, RGBAColor *colour, int *index)
{
    int iycoor;
    int ixcoor;
    int i;
    int cornersIndex[4];
    double indexCrn[4];
    RGBAColor cornerColour[4];
    double redCrn[4];
    double greenCrn[4];
    double blueCrn[4];
    double alphaCrn[4];
    double val1 = 0, val2 = 0, val3 = 0, val4 = 0;

    iycoor = (int)ycoor;
    ixcoor = (int)xcoor;
    for (i = 0; i < 4; i++) {
        Color::makeColor(&cornerColour[i], 0.0, 0.0, 0.0);
        cornerColour[i].Alpha = 0.0;
    }
    /* OK, now that you have the corners, what are you going to do with them? */
    if (image->Interpolation_Type == BILINEAR) {
        noInterpolation(image, (double)ixcoor + 1, (double)iycoor,
            &cornerColour[0], &cornersIndex[0]);
        noInterpolation(image, (double)ixcoor, (double)iycoor, &cornerColour[1],
            &cornersIndex[1]);
        noInterpolation(image, (double)ixcoor + 1, (double)iycoor - 1,
            &cornerColour[2], &cornersIndex[2]);
        noInterpolation(image, (double)ixcoor, (double)iycoor - 1,
            &cornerColour[3], &cornersIndex[3]);
        for (i = 0; i < 4; i++) {
            redCrn[i] = cornerColour[i].Red;
            greenCrn[i] = cornerColour[i].Green;
            blueCrn[i] = cornerColour[i].Blue;
            alphaCrn[i] = cornerColour[i].Alpha;
            /* Logger::info("Crn %d = %lf %lf
             * %lf\n",i,Red_Crn[i],Blue_Crn[i],Green_Crn[i]); */
        }

        val1 = bilinear(redCrn, xcoor, ycoor);
        val2 = bilinear(greenCrn, xcoor, ycoor);
        val3 = bilinear(blueCrn, xcoor, ycoor);
        val4 = bilinear(alphaCrn, xcoor, ycoor);
    }
    if (image->Interpolation_Type == NORMALIZED_DIST) {
        noInterpolation(image, (double)ixcoor, (double)iycoor - 1,
            &cornerColour[0], &cornersIndex[0]);
        noInterpolation(image, (double)ixcoor + 1, (double)iycoor - 1,
            &cornerColour[1], &cornersIndex[1]);
        noInterpolation(image, (double)ixcoor, (double)iycoor, &cornerColour[2],
            &cornersIndex[2]);
        noInterpolation(image, (double)ixcoor + 1, (double)iycoor,
            &cornerColour[3], &cornersIndex[3]);
        for (i = 0; i < 4; i++) {
            redCrn[i] = cornerColour[i].Red;
            greenCrn[i] = cornerColour[i].Green;
            blueCrn[i] = cornerColour[i].Blue;
            alphaCrn[i] = cornerColour[i].Alpha;
            /* Logger::info("Crn %d = %lf %lf
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
    /* Logger::info("Final = %lf %lf %lf\n",val1,val2,val3);  */
    /* use bilinear for index try average later */
    for (i = 0; i < 4; i++) {
        indexCrn[i] = (double)cornersIndex[i];
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
double
MapTextureFixture::bilinear(double *corners, double x, double y)
{
    double p, q;
    double val = 0.0;

    p = x - (int)x;
    q = y - (int)y;
    if ((p == 0.0) && (q == 0.0)) {
        return (*corners); /* upper left */
    }

    val = (p * q * *corners) + (q * (1 - p) * *(corners + 1)) +
          (p * (1 - q) * *(corners + 2)) + ((1 - p) * (1 - q) * *(corners + 3));
    return (val);
}

static constexpr int MAX_PTS = 4;

inline double
MapTextureFixture::MapTextureFixture::pythagoreanSq(double a, double b)
{
    return a * a + b * b;
}

double
MapTextureFixture::normDist(double *corners, double x, double y)
{
    int i;

    double p, q;
    double wts[MAX_PTS];
    double sumInvWts = 0.0;
    double sumI = 0.0;

    p = x - (int)x;
    q = y - (int)y;

    if ((p == 0.0) && (q == 0.0)) {
        return (*corners); /* upper left */
    }

    wts[0] = MapTextureFixture::pythagoreanSq(p, q);
    wts[1] = MapTextureFixture::pythagoreanSq(1 - p, q);
    wts[2] = MapTextureFixture::pythagoreanSq(p, 1 - q);
    wts[3] = MapTextureFixture::pythagoreanSq(1 - p, 1 - q);

    for (i = 0; i < MAX_PTS; i++) {
        sumInvWts += 1 / wts[i];
        sumI += *(corners + i) / wts[i];
    }

    return (sumI / sumInvWts);
}
