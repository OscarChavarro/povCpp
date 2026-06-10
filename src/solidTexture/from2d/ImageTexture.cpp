/**
Implements mapped textures: image map, bump map, and material map.
Supports planar, spherical, cylindrical, and torus UV projections.
*/

#include "solidTexture/from2d/ImageTexture.h"
#include "solidTexture/from2d/ImageToSolidTextureInterpolationTypes.h"
#include "solidTexture/from2d/ImageToSolidTextureProjectionMethods.h"
#include "solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"

/**
2-D to 3-D procedural texture mapping of a bitmap onto an object.
Planar method by DKB and AAC: transform, find 2-D coords from 3-D, return pixel
color. Specialized projections (cylindrical, spherical, torus) by Alexander
Enzmann.
*/
void
ImageTexture::imageMap(
    double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, ColorRgba *color,
    double smallTolerance)
{
    double xCoordinate = 0.0;
    double yCoordinate = 0.0;
    int regNumber;

    if (map(x, y, z, image, &xCoordinate, &yCoordinate, smallTolerance)) {
        color->setR(1.0); color->setG(1.0); color->setB(1.0); color->setA(1.0);
        return;
    }
    imageColorAt(image, xCoordinate, yCoordinate, color, &regNumber);
}

/**
Takes an intersection point and a texture; returns a new texture based on
the index/color of that point in an image/materials map.
*/
int
ImageTexture::materialMap(
    Vector3Dd *intersectionPoint,
    Matrix4x4d *textureTransformationInverse,
    ControlledRGBAImageHDRUncompressed *materialImage,
    int numberOfMaterials,
    double smallTolerance)
{
    Vector3Dd transformedPoint;
    double x;
    double y;
    double z;
    double xCoordinate = 0.0;
    double yCoordinate = 0.0;
    int regNumber = 0;
    ColorRgba color;
    int materialNumber = 0;
    color.setR(0.0); color.setG(0.0); color.setB(0.0); color.setA(0.0);

    if (textureTransformationInverse) {
        transformedPoint = textureTransformationInverse->transpose().multiply(
            *intersectionPoint);
    } else {
        transformedPoint = *intersectionPoint;
    }

    x = transformedPoint.x();
    y = transformedPoint.y();
    z = transformedPoint.z();

    // Now we have transformed x, y, z: use image mapping to determine texture index
    if (map(x, y, z, materialImage, &xCoordinate, &yCoordinate, smallTolerance)) {
        materialNumber = 0;
    } else {
        imageColorAt(materialImage, xCoordinate, yCoordinate, &color, &regNumber);

        if (materialImage->getIndexedData() == nullptr) {
            materialNumber = (int)color.getR() * 255;
        } else {
            materialNumber = regNumber;
        }
    }

    if (numberOfMaterials > 0 && materialNumber >= numberOfMaterials) {
        materialNumber %= numberOfMaterials;
    }
    if (materialNumber < numberOfMaterials) {
        return materialNumber;
    }
    return -1;
}

void
ImageTexture::bumpMap(
    double x, double y, double z, ControlledRGBAImageHDRUncompressed *bumpImage, double bumpAmount,
    Vector3Dd *normal, double smallTolerance)
{
    double xcoor = 0.0;
    double ycoor = 0.0;
    int index;
    int index2;
    int index3;
    ColorRgba color;
    ColorRgba color2;
    ColorRgba color3;
    Vector3Dd p1;
    Vector3Dd p2;
    Vector3Dd p3;
    Vector3Dd bumpNormal;
    Vector3Dd xprime;
    Vector3Dd yprime;
    Vector3Dd zprime;
    Vector3Dd temp;
    double length;
    color.setR(0.0); color.setG(0.0); color.setB(0.0); color.setA(0.0);
    color2.setR(0.0); color2.setG(0.0); color2.setB(0.0); color2.setA(0.0);
    color3.setR(0.0); color3.setG(0.0); color3.setB(0.0); color3.setA(0.0);

    // going to have to change this
    // need to know if bump point is off of image for all 3 points

    if (map(x, y, z, bumpImage, &xcoor, &ycoor, smallTolerance)) {
        color.setR(1.0); color.setG(1.0); color.setB(1.0); color.setA(1.0);
        index = 255;
        return;
    }
    imageColorAt(bumpImage, xcoor, ycoor, &color, &index);

    xcoor--;
    ycoor++;
    if (xcoor < 0.0) {
        xcoor += (double)bumpImage->getXSize();
    } else if (xcoor >= bumpImage->getXSize()) {
        xcoor -= (double)bumpImage->getXSize();
    }
    if (ycoor < 0.0) {
        ycoor += (double)bumpImage->getYSize();
    } else if (ycoor >= (double)bumpImage->getYSize()) {
        ycoor -= (double)bumpImage->getYSize();
    }
    imageColorAt(
        bumpImage, xcoor, ycoor, &color2, &index2);

    xcoor += 2.0;
    if (xcoor < 0.0) {
        xcoor += (double)bumpImage->getXSize();
    } else if (xcoor >= bumpImage->getXSize()) {
        xcoor -= (double)bumpImage->getXSize();
    }

    imageColorAt(
        bumpImage, xcoor, ycoor, &color3, &index3);


    if (bumpImage->getIndexedData() == nullptr ||
        bumpImage->getUseColorFlag()) {
        p1 = Vector3Dd(0,
            bumpAmount *
                (0.229 * color.getR() + 0.587 * color.getG() + 0.114 * color.getB()),
            0);
        p2 = Vector3Dd(0,
            bumpAmount *
                (0.229 * color2.getR() + 0.587 * color2.getG() +
                    0.114 * color2.getB()),
            1);
        p3 = Vector3Dd(1,
            bumpAmount *
                (0.229 * color3.getR() + 0.587 * color3.getG() +
                    0.114 * color3.getB()),
            1);
    } else {
        p1 = Vector3Dd(0, bumpAmount * index, 0);
        p2 = Vector3Dd(0, bumpAmount * index2, 1);
        p3 = Vector3Dd(1, bumpAmount * index3, 1);
    }
    // we have points 1,2,3 for a triangle; compute the surface normal
    xprime = p1.subtract(p2);
    yprime = p3.subtract(p2);
    bumpNormal = yprime.crossProduct(xprime);
    bumpNormal = bumpNormal.normalizedFast();

    *&yprime = Vector3Dd(normal->x(), normal->y(), normal->z());
    *&temp = Vector3Dd(0.0, 1.0, 0.0);
    xprime = yprime.crossProduct(temp);
    length = xprime.length();
    if (length < 1.0e-9) {
        if (fabs(normal->y() - 1.0) < smallTolerance) {
            *&yprime = Vector3Dd(0.0, 1.0, 0.0);
            *&xprime = Vector3Dd(1.0, 0.0, 0.0);
            length = 1.0;
        } else {
            *&yprime = Vector3Dd(0.0, -1.0, 0.0);
            *&xprime = Vector3Dd(1.0, 0.0, 0.0);
            length = 1.0;
        }
    }
    xprime = xprime.multiply(1.0 / length);
    zprime = xprime.crossProduct(yprime);
    zprime = zprime.normalizedFast();
    xprime = xprime.multiply(bumpNormal.x());
    yprime = yprime.multiply(bumpNormal.y());
    zprime = zprime.multiply(bumpNormal.z());
    temp = xprime.add(yprime);
    *normal = temp.add(zprime);
    *normal = (*normal).normalizedFast();
}

void
ImageTexture::imageColorAt(
    ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate, ColorRgba *color, int *index)
{
    switch (image->getInterpolationType()) {
    case (int)ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION:
        noInterpolation(image, xCoordinate, yCoordinate, color, index);
        break;
    default:
        interp(image, xCoordinate, yCoordinate, color, index);
        break;
    }
}

/** Maps a point (x, y, z) on a cylinder of radius 1, height 1 with y-axis symmetry to [0,1]x[0,1]. */
int
ImageTexture::cylindricalImageMap(
    double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, double *u, double *v)
{
    double len;
    double theta;

    if ((image->getOnceFlag()) && ((y < 0.0) || (y > 1.0))) {
        return 0;
    }
    *v = fmod(y * image->getYSize(), image->getYSize());

    // Make sure this vector is on the unit sphere.
    len = sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        return 0;
    }
    x /= len;
    z /= len;

    // Determine its angle from the point (1, 0, 0) in the x-z plane.
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
    theta /= 2.0 * M_PI; // normalizes theta to [0, 1]

    *u = (theta * image->getXSize());
    return 1;
}

/** Maps a point (x, y, z) on a torus to a 2-D image. */
int
ImageTexture::torusImageMap(
    double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, double *u, double *v)
{
    double len;
    double phi;
    double theta;
    double r0;

    r0 = image->getImageGradient().x();

    // Determine its angle from the x-axis.
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

    // Rotate about the y-axis to get the point (x, y, z) into the x-y plane.
    x = len - r0;
    len = sqrt(x * x + y * y);
    phi = acos(-x / len);
    if (y > 0.0) {
        phi = 2.0 * M_PI - phi;
    }

    // Determine the parametric coordinates.
    theta /= 2.0 * M_PI;
    phi /= 2.0 * M_PI;
    *u = (-theta * image->getXSize());
    *v = (phi * image->getYSize());
    return 1;
}

/** Maps a point (x, y, z) on a unit sphere to a 2-D image. */
int
ImageTexture::sphericalImageMap(
    double x, double y, double z, RGBAImageHDRUncompressed *image, double *u, double *v)
{
    double len;
    double phi;
    double theta;

    // Make sure this vector is on the unit sphere.
    len = sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        return 0;
    }
    x /= len;
    y /= len;
    z /= len;

    // Determine its angle from the x-z plane.
    phi = 0.5 + asin(y) / M_PI; // normalizes phi to [0, 1]

    // Determine its angle from the point (1, 0, 0) in the x-z plane.
    len = sqrt(x * x + z * z);
    if (len == 0.0) {
        // at a pole: any xcoord value is valid
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
        theta /= 2.0 * M_PI; // normalizes theta to [0, 1]
    }
    *u = (theta * image->getXSize());
    *v = (phi * image->getYSize());
    return 1;
}

/**
Simplistic planar image projection by DKB and AAC.
Returns 0 if no color at this point (invisible), 1 if a good mapping is found.
*/
int
ImageTexture::planarImageMap(
    double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, double *u, double *v)
{
    if (image->getImageGradient().x() != 0.0) {
        if ((image->getOnceFlag()) && ((x < 0.0) || (x > 1.0))) {
            return 0;
        }
        if (image->getImageGradient().x() > 0) {
            *u = fmod(x * image->getXSize(), image->getXSize());
        } else {
            *v = fmod(x * image->getYSize(), image->getYSize());
        }
    }
    if (image->getImageGradient().y() != 0.0) {
        if ((image->getOnceFlag()) && ((y < 0.0) || (y > 1.0))) {
            return 0;
        }
        if (image->getImageGradient().y() > 0) {
            *u = fmod(y * image->getXSize(), image->getXSize());
        } else {
            *v = fmod(y * image->getYSize(), image->getYSize());
        }
    }
    if (image->getImageGradient().z() != 0.0) {
        if ((image->getOnceFlag()) && ((z < 0.0) || (z > 1.0))) {
            return 0;
        }
        if (image->getImageGradient().z() > 0) {
            *u = fmod(z * image->getXSize(), image->getXSize());
        } else {
            *v = fmod(z * image->getYSize(), image->getYSize());
        }
    }
    return 1;
}

/** Returns 1 if no color found at this point (invisible), 0 if a color was mapped. */
int
ImageTexture::map(double x, double y, double z, ControlledRGBAImageHDRUncompressed *image,
    double *xCoordinate, double *yCoordinate, double smallTolerance)
{
    // Now determine which mapper to use.
    switch (image->getMapType()) {
    case (int)ImageToSolidTextureProjectionMethods::PLANAR_MAP:
        if (!planarImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return (1);
        }
        break;
    case (int)ImageToSolidTextureProjectionMethods::SPHERICAL_MAP:
        if (!sphericalImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return (1);
        }
        break;
    case (int)ImageToSolidTextureProjectionMethods::CYLINDRICAL_MAP:
        if (!cylindricalImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return (1);
        }
        break;
    case (int)ImageToSolidTextureProjectionMethods::TORUS_MAP:
        if (!torusImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return (1);
        }
        break;
    default:
        if (!planarImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return (1);
        }
        break;
    }
    // make sure the point is on the image
    *yCoordinate += smallTolerance;
    *xCoordinate += smallTolerance;
    // compensate for y coordinates on images being upside-down
    *yCoordinate = (double)image->getYSize() - *yCoordinate;

    if (*xCoordinate < 0.0) {
        *xCoordinate += (double)image->getXSize();
    } else if (*xCoordinate >= (double)image->getXSize()) {
        *xCoordinate -= (double)image->getXSize();
    }

    if (*yCoordinate < 0.0) {
        *yCoordinate += (double)image->getYSize();
    } else if (*yCoordinate >= (double)image->getYSize()) {
        *yCoordinate -= (double)image->getYSize();
    }

    if ((*xCoordinate >= (double)image->getXSize()) ||
        (*yCoordinate >= (double)image->getYSize()) || (*xCoordinate < 0.0) ||
        (*yCoordinate < 0.0)) {
        Logger::reportMessage("MapTextureFixture", Logger::FATAL_ERROR, "", "\nPicture index out of range\n");
    }

    return (0);
}

void
ImageTexture::noInterpolation(
    ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate, ColorRgba *color, int *index)
{
    if (xCoordinate < 0.0) {
        xCoordinate += (double)image->getXSize();
    } else if (xCoordinate >= (double)image->getXSize()) {
        xCoordinate -= (double)image->getXSize();
    }
    if (yCoordinate < 0.0) {
        yCoordinate += (double)image->getYSize();
    } else if (yCoordinate >= (double)image->getYSize()) {
        yCoordinate -= (double)image->getYSize();
    }

    int iycoor = (int)yCoordinate;
    int ixcoor = (int)xCoordinate;

    if (image->getIndexedData() == nullptr) {
        RGBAPixelHDR pixel;
        image->getPixel(ixcoor, iycoor, &pixel);
        color->setR(color->getR() + (double)pixel.r / 255.0);
        color->setG(color->getG() + (double)pixel.g / 255.0);
        color->setB(color->getB() + (double)pixel.b / 255.0);
        *index = -1;
    } else {
        IndexedColorImageHDRUncompressed *idx = image->getIndexedData();
        *index = idx->getPixel(ixcoor, iycoor);
        RGBAPixelHDR *mapColor = &idx->getColorTable()[*index];
        color->setR(color->getR() + (double)mapColor->r / 255.0);
        color->setG(color->getG() + (double)mapColor->g / 255.0);
        color->setB(color->getB() + (double)mapColor->b / 255.0);
        color->setA(color->getA() + (double)mapColor->a / 255.0);
    }
}

/** Interpolates color and alpha values when mapping. */
void
ImageTexture::interp(
    ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate, ColorRgba *color, int *index)
{
    int iycoor;
    int ixcoor;
    int i;
    int cornersIndex[4];
    double indexCrn[4];
    ColorRgba cornerColor[4];
    double redCrn[4];
    double greenCrn[4];
    double blueCrn[4];
    double alphaCrn[4];
    double val1 = 0;
    double val2 = 0;
    double val3 = 0;
    double val4 = 0;

    iycoor = (int)yCoordinate;
    ixcoor = (int)xCoordinate;
    for (i = 0; i < 4; i++) {
        cornerColor[i].setR(0.0); cornerColor[i].setG(0.0); cornerColor[i].setB(0.0); cornerColor[i].setA(0.0);
    }

    // Sample the four surrounding pixels
    if (image->getInterpolationType() == (int)ImageToSolidTextureInterpolationTypes::BILINEAR) {
        noInterpolation(image, (double)ixcoor + 1, (double)iycoor,
            &cornerColor[0], &cornersIndex[0]);
        noInterpolation(image, (double)ixcoor, (double)iycoor, &cornerColor[1],
            &cornersIndex[1]);
        noInterpolation(image, (double)ixcoor + 1, (double)iycoor - 1,
            &cornerColor[2], &cornersIndex[2]);
        noInterpolation(image, (double)ixcoor, (double)iycoor - 1,
            &cornerColor[3], &cornersIndex[3]);
        for (i = 0; i < 4; i++) {
            redCrn[i] = cornerColor[i].getR();
            greenCrn[i] = cornerColor[i].getG();
            blueCrn[i] = cornerColor[i].getB();
            alphaCrn[i] = cornerColor[i].getA();
        }

        val1 = biLinear(redCrn, xCoordinate, yCoordinate);
        val2 = biLinear(greenCrn, xCoordinate, yCoordinate);
        val3 = biLinear(blueCrn, xCoordinate, yCoordinate);
        val4 = biLinear(alphaCrn, xCoordinate, yCoordinate);
    }
    if (image->getInterpolationType() == (int)ImageToSolidTextureInterpolationTypes::NORMALIZED_DIST) {
        noInterpolation(image, (double)ixcoor, (double)iycoor - 1,
            &cornerColor[0], &cornersIndex[0]);
        noInterpolation(image, (double)ixcoor + 1, (double)iycoor - 1,
            &cornerColor[1], &cornersIndex[1]);
        noInterpolation(image, (double)ixcoor, (double)iycoor, &cornerColor[2],
            &cornersIndex[2]);
        noInterpolation(image, (double)ixcoor + 1, (double)iycoor,
            &cornerColor[3], &cornersIndex[3]);
        for (i = 0; i < 4; i++) {
            redCrn[i] = cornerColor[i].getR();
            greenCrn[i] = cornerColor[i].getG();
            blueCrn[i] = cornerColor[i].getB();
            alphaCrn[i] = cornerColor[i].getA();
            // Logger::info("Crn %d = %lf %lf %lf\n",i,Red_Crn[i],Blue_Crn[i],Green_Crn[i]);
        }

        val1 = normDist(redCrn, xCoordinate, yCoordinate);
        val2 = normDist(greenCrn, xCoordinate, yCoordinate);
        val3 = normDist(blueCrn, xCoordinate, yCoordinate);
        val4 = normDist(alphaCrn, xCoordinate, yCoordinate);
    }

    color->setR(color->getR() + val1);
    color->setG(color->getG() + val2);
    color->setB(color->getB() + val3);
    color->setA(color->getA() + val4);
    // Logger::info("Final = %lf %lf %lf\n",val1,val2,val3);
    // use bilinear for index; try average later
    for (i = 0; i < 4; i++) {
        indexCrn[i] = (double)cornersIndex[i];
    }
    if (image->getInterpolationType() == (int)ImageToSolidTextureInterpolationTypes::BILINEAR) {
        *index = (int)(biLinear(indexCrn, xCoordinate, yCoordinate) + 0.5);
    }
    if (image->getInterpolationType() == (int)ImageToSolidTextureInterpolationTypes::NORMALIZED_DIST) {
        *index = (int)(normDist(indexCrn, xCoordinate, yCoordinate) + 0.5);
    }
}

/**
Bilinear interpolation. From an article by Girish T. Hagan in
C Programmer's Journal V 9 No. 8; adapted for POV-Ray by CdW.
*/
double
ImageTexture::biLinear(double *corners, double x, double y)
{
    double p;
    double q;
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
ImageTexture::pythagoreanSq(double a, double b)
{
    return a * a + b * b;
}

double
ImageTexture::normDist(double *corners, double x, double y)
{
    double p;
    double q;
    double wts[MAX_PTS];
    double sumInvWts = 0.0;
    double sumI = 0.0;

    p = x - (int)x;
    q = y - (int)y;

    if ((p == 0.0) && (q == 0.0)) {
        return (*corners); /* upper left */
    }

    wts[0] = pythagoreanSq(p, q);
    wts[1] = pythagoreanSq(1 - p, q);
    wts[2] = pythagoreanSq(p, 1 - q);
    wts[3] = pythagoreanSq(1 - p, 1 - q);

    for (int i = 0; i < MAX_PTS; i++) {
        sumInvWts += 1 / wts[i];
        sumI += *(corners + i) / wts[i];
    }

    return sumI / sumInvWts;
}
