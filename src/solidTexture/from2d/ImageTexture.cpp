/**
Implements mapped textures: image map, bump map, and material map.
Supports planar, spherical, cylindrical, and torus UV projections.
*/

#include <cmath>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "solidTexture/from2d/ImageTexture.h"
#include "solidTexture/from2d/ImageToSolidTextureInterpolationTypes.h"
#include "solidTexture/from2d/ImageToSolidTextureProjectionMethods.h"
#include "solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"

static constexpr int MAX_PTS = 4;

/**
2-D to 3-D procedural texture mapping of a bitmap onto an object.
Planar method by DKB and AAC: transform, find 2-D coords from 3-D, return pixel
color. Specialized projections (cylindrical, spherical, torus) by Alexander
Enzmann.
*/
void
ImageTexture::imageMap(
    double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, ColorRgba *color,
    double smallTolerance) const
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
    const Vector3Dd *intersectionPoint,
    const Matrix4x4d *textureTransformationInverse,
    const ControlledRGBAImageHDRUncompressed *materialImage,
    int numberOfMaterials,
    double smallTolerance) const
{
    Vector3Dd transformedPoint;
    double x;
    double y;
    double z;
    double xCoordinate = 0.0;
    double yCoordinate = 0.0;
    int regNumber = 0;
    ColorRgba color;
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
    int materialNumber;

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
    double x, double y, double z, const ControlledRGBAImageHDRUncompressed *bumpImage, double bumpAmount,
    Vector3Dd *normal, double smallTolerance) const
{
    double xCoordinate = 0.0;
    double yCoordinate = 0.0;
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
    Vector3Dd xPrime;
    Vector3Dd yPrime;
    Vector3Dd zPrime;
    Vector3Dd temp;
    double length;
    color.setR(0.0); color.setG(0.0); color.setB(0.0); color.setA(0.0);
    color2.setR(0.0); color2.setG(0.0); color2.setB(0.0); color2.setA(0.0);
    color3.setR(0.0); color3.setG(0.0); color3.setB(0.0); color3.setA(0.0);

    // going to have to change this
    // need to know if bump point is off of image for all 3 points

    if (map(x, y, z, bumpImage, &xCoordinate, &yCoordinate, smallTolerance)) {
        color.setR(1.0); color.setG(1.0); color.setB(1.0); color.setA(1.0);
        index = 255;
        return;
    }
    imageColorAt(bumpImage, xCoordinate, yCoordinate, &color, &index);

    xCoordinate--;
    yCoordinate++;
    if (xCoordinate < 0.0) {
        xCoordinate += (double)bumpImage->getXSize();
    } else if (xCoordinate >= bumpImage->getXSize()) {
        xCoordinate -= (double)bumpImage->getXSize();
    }
    if (yCoordinate < 0.0) {
        yCoordinate += (double)bumpImage->getYSize();
    } else if (yCoordinate >= (double)bumpImage->getYSize()) {
        yCoordinate -= (double)bumpImage->getYSize();
    }
    imageColorAt(
        bumpImage, xCoordinate, yCoordinate, &color2, &index2);

    xCoordinate += 2.0;
    if (xCoordinate < 0.0) {
        xCoordinate += (double)bumpImage->getXSize();
    } else if (xCoordinate >= bumpImage->getXSize()) {
        xCoordinate -= (double)bumpImage->getXSize();
    }

    imageColorAt(
        bumpImage, xCoordinate, yCoordinate, &color3, &index3);

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

    // We have points 1, 2, 3 for a triangle; compute the surface normal
    xPrime = p1.subtract(p2);
    yPrime = p3.subtract(p2);
    bumpNormal = yPrime.crossProduct(xPrime);
    bumpNormal = bumpNormal.normalizedFast();

    *&yPrime = Vector3Dd(normal->x(), normal->y(), normal->z());
    *&temp = Vector3Dd(0.0, 1.0, 0.0);
    xPrime = yPrime.crossProduct(temp);
    length = xPrime.length();
    if (length < 1.0e-9) {
        if (java::Math::abs(normal->y() - 1.0) < smallTolerance) {
            *&yPrime = Vector3Dd(0.0, 1.0, 0.0);
            *&xPrime = Vector3Dd(1.0, 0.0, 0.0);
            length = 1.0;
        } else {
            *&yPrime = Vector3Dd(0.0, -1.0, 0.0);
            *&xPrime = Vector3Dd(1.0, 0.0, 0.0);
            length = 1.0;
        }
    }
    xPrime = xPrime.multiply(1.0 / length);
    zPrime = xPrime.crossProduct(yPrime);
    zPrime = zPrime.normalizedFast();
    xPrime = xPrime.multiply(bumpNormal.x());
    yPrime = yPrime.multiply(bumpNormal.y());
    zPrime = zPrime.multiply(bumpNormal.z());
    temp = xPrime.add(yPrime);
    *normal = temp.add(zPrime);
    *normal = (*normal).normalizedFast();
}

void
ImageTexture::imageColorAt(
    const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate, ColorRgba *color, int *index) const
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

/**
Maps a point (x, y, z) on a cylinder of radius 1, height 1 with y-axis symmetry to [0,1]x[0,1].
*/
bool
ImageTexture::cylindricalImageMap(
    double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const
{
    double len;
    double theta;

    if ((image->getOnceFlag()) && ((y < 0.0) || (y > 1.0))) {
        return false;
    }
    *v = std::fmod(y * image->getYSize(), image->getYSize());

    // Make sure this vector is on the unit sphere.
    len = java::Math::sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        return false;
    }
    x /= len;
    z /= len;

    // Determine its angle from the point (1, 0, 0) in the x-z plane.
    len = java::Math::sqrt(x * x + z * z);
    if (len == 0.0) {
        return false;
    }
    if (z == 0.0) {
        if (x > 0) {
            theta = 0.0;
        } else {
            theta = java::Math::PI;
        }
    } else {
        theta = java::Math::acos(x / len);
        if (z < 0.0) {
            theta = 2.0 * java::Math::PI - theta;
        }
    }
    theta /= 2.0 * java::Math::PI; // normalizes theta to [0, 1]

    *u = (theta * image->getXSize());
    return true;
}

/**
Maps a point (x, y, z) on a torus to a 2-D image.
*/
bool
ImageTexture::torusImageMap(
    double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const
{
    double len;
    double phi;
    double theta;
    double r0;

    r0 = image->getImageGradient().x();

    // Determine its angle from the x-axis.
    len = java::Math::sqrt(x * x + z * z);
    if (len == 0.0) {
        return false;
    }
    if (z == 0.0) {
        if (x > 0) {
            theta = 0.0;
        } else {
            theta = java::Math::PI;
        }
    } else {
        theta = java::Math::acos(x / len);
        if (z < 0.0) {
            theta = 2.0 * java::Math::PI - theta;
        }
    }

    theta = 0.0 - theta;

    // Rotate about the y-axis to get the point (x, y, z) into the x-y plane.
    x = len - r0;
    len = java::Math::sqrt(x * x + y * y);
    phi = java::Math::acos(-x / len);
    if (y > 0.0) {
        phi = 2.0 * java::Math::PI - phi;
    }

    // Determine the parametric coordinates.
    theta /= 2.0 * java::Math::PI;
    phi /= 2.0 * java::Math::PI;
    *u = (-theta * image->getXSize());
    *v = (phi * image->getYSize());
    return true;
}

/** Maps a point (x, y, z) on a unit sphere to a 2-D image. */
bool
ImageTexture::sphericalImageMap(
    double x, double y, double z, const RGBAImageHDRUncompressed *image, double *u, double *v) const
{
    double len;
    double phi;
    double theta;

    // Make sure this vector is on the unit sphere.
    len = java::Math::sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        return false;
    }
    x /= len;
    y /= len;
    z /= len;

    // Determine its angle from the x-z plane.
    phi = 0.5 + java::Math::asin(y) / java::Math::PI; // normalizes phi to [0, 1]

    // Determine its angle from the point (1, 0, 0) in the x-z plane.
    len = java::Math::sqrt(x * x + z * z);
    if (len == 0.0) {
        // At a pole: any xCoordinate value is valid
        theta = 0;
    } else {
        if (z == 0.0) {
            if (x > 0) {
                theta = 0.0;
            } else {
                theta = java::Math::PI;
            }
        } else {
            theta = java::Math::acos(x / len);
            if (z < 0.0) {
                theta = 2.0 * java::Math::PI - theta;
            }
        }
        theta /= 2.0 * java::Math::PI; // normalizes theta to [0, 1]
    }
    *u = (theta * image->getXSize());
    *v = (phi * image->getYSize());
    return true;
}

/**
Simplistic planar image projection by DKB and AAC.
Returns 0 if no color at this point (invisible), 1 if a good mapping is found.
*/
bool
ImageTexture::planarImageMap(
    double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const
{
    if (image->getImageGradient().x() != 0.0) {
        if ((image->getOnceFlag()) && ((x < 0.0) || (x > 1.0))) {
            return false;
        }
        if (image->getImageGradient().x() > 0) {
            *u = std::fmod(x * image->getXSize(), image->getXSize());
        } else {
            *v = std::fmod(x * image->getYSize(), image->getYSize());
        }
    }
    if (image->getImageGradient().y() != 0.0) {
        if ((image->getOnceFlag()) && ((y < 0.0) || (y > 1.0))) {
            return false;
        }
        if (image->getImageGradient().y() > 0) {
            *u = std::fmod(y * image->getXSize(), image->getXSize());
        } else {
            *v = std::fmod(y * image->getYSize(), image->getYSize());
        }
    }
    if (image->getImageGradient().z() != 0.0) {
        if ((image->getOnceFlag()) && ((z < 0.0) || (z > 1.0))) {
            return false;
        }
        if (image->getImageGradient().z() > 0) {
            *u = std::fmod(z * image->getXSize(), image->getXSize());
        } else {
            *v = std::fmod(z * image->getYSize(), image->getYSize());
        }
    }
    return true;
}

/**
Returns 1 if no color found at this point (invisible), 0 if a color was mapped.
*/
bool
ImageTexture::map(double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image,
    double *xCoordinate, double *yCoordinate, double smallTolerance) const
{
    // Now determine which mapper to use.
    switch (image->getMapType()) {
    case ImageToSolidTextureProjectionMethods::PLANAR_MAP:
        if (!planarImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return true;
        }
        break;
    case ImageToSolidTextureProjectionMethods::SPHERICAL_MAP:
        if (!sphericalImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return true;
        }
        break;
    case ImageToSolidTextureProjectionMethods::CYLINDRICAL_MAP:
        if (!cylindricalImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return true;
        }
        break;
    case ImageToSolidTextureProjectionMethods::TORUS_MAP:
        if (!torusImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return true;
        }
        break;
    default:
        if (!planarImageMap(x, y, z, image, xCoordinate, yCoordinate)) {
            return true;
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

    return false;
}

void
ImageTexture::noInterpolation(
    const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate, ColorRgba *color, int *index) const
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

    const int x = (int)xCoordinate;
    const int y = (int)yCoordinate;

    if (image->getIndexedData() == nullptr) {
        RGBAPixelHDR pixel;
        image->getPixel(x, y, &pixel);
        color->setR(color->getR() + (double)pixel.r / 255.0);
        color->setG(color->getG() + (double)pixel.g / 255.0);
        color->setB(color->getB() + (double)pixel.b / 255.0);
        *index = -1;
    } else {
        const IndexedColorImageHDRUncompressed *idx = image->getIndexedData();
        *index = idx->getPixel(x, y);
        const RGBAPixelHDR *mapColor = &idx->getColorTable()[*index];
        color->setR(color->getR() + (double)mapColor->r / 255.0);
        color->setG(color->getG() + (double)mapColor->g / 255.0);
        color->setB(color->getB() + (double)mapColor->b / 255.0);
        color->setA(color->getA() + (double)mapColor->a / 255.0);
    }
}

/**
Interpolates color and alpha values when mapping.
*/
void
ImageTexture::interp(
    const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate, ColorRgba *color, int *index) const
{
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
    int x = (int)xCoordinate;
    int y = (int)yCoordinate;

    int i;

    for (i = 0; i < 4; i++) {
        cornerColor[i].setR(0.0); cornerColor[i].setG(0.0); cornerColor[i].setB(0.0); cornerColor[i].setA(0.0);
    }

    // Sample the four surrounding pixels
    if (image->getInterpolationType() == ImageToSolidTextureInterpolationTypes::BI_LINEAR) {
        noInterpolation(image, (double)x + 1, (double)y,
            &cornerColor[0], &cornersIndex[0]);
        noInterpolation(image, (double)x, (double)y, &cornerColor[1],
            &cornersIndex[1]);
        noInterpolation(image, (double)x + 1, (double)y - 1,
            &cornerColor[2], &cornersIndex[2]);
        noInterpolation(image, (double)x, (double)y - 1,
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
        noInterpolation(image, (double)x, (double)y - 1,
            &cornerColor[0], &cornersIndex[0]);
        noInterpolation(image, (double)x + 1, (double)y - 1,
            &cornerColor[1], &cornersIndex[1]);
        noInterpolation(image, (double)x, (double)y, &cornerColor[2],
            &cornersIndex[2]);
        noInterpolation(image, (double)x + 1, (double)y,
            &cornerColor[3], &cornersIndex[3]);
        for (i = 0; i < 4; i++) {
            redCrn[i] = cornerColor[i].getR();
            greenCrn[i] = cornerColor[i].getG();
            blueCrn[i] = cornerColor[i].getB();
            alphaCrn[i] = cornerColor[i].getA();
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

    // Use bi-linear for index; try average later
    for (i = 0; i < 4; i++) {
        indexCrn[i] = (double)cornersIndex[i];
    }
    if (image->getInterpolationType() == (int)ImageToSolidTextureInterpolationTypes::BI_LINEAR) {
        *index = (int)(biLinear(indexCrn, xCoordinate, yCoordinate) + 0.5);
    }
    if (image->getInterpolationType() == (int)ImageToSolidTextureInterpolationTypes::NORMALIZED_DIST) {
        *index = (int)(normDist(indexCrn, xCoordinate, yCoordinate) + 0.5);
    }
}

/**
Bi-linear interpolation. From an article by Girish T. Hagan in
C Programmer's Journal V 9 No. 8; adapted for POV-Ray by CdW.
*/
double
ImageTexture::biLinear(const double *corners, double x, double y) const
{
    double p;
    double q;

    p = x - (int)x;
    q = y - (int)y;
    if ((p == 0.0) && (q == 0.0)) {
        return (*corners); // Upper left
    }

    return (p * q * *corners) + (q * (1 - p) * *(corners + 1)) +
          (p * (1 - q) * *(corners + 2)) + ((1 - p) * (1 - q) * *(corners + 3));
}

inline double
ImageTexture::pythagoreanSq(double a, double b) const
{
    return a * a + b * b;
}

double
ImageTexture::normDist(const double *corners, double x, double y) const
{
    double p;
    double q;
    double wts[MAX_PTS];
    double sumInvWts = 0.0;
    double sumI = 0.0;

    p = x - (int)x;
    q = y - (int)y;

    if ((p == 0.0) && (q == 0.0)) {
        return (*corners); // Upper left
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
