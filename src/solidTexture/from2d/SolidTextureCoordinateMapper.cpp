#include <cmath>

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "solidTexture/from2d/ImageToSolidTextureProjectionMethods.h"
#include "solidTexture/from2d/SolidTextureCoordinateMapper.h"

bool
SolidTextureCoordinateMapper::cylindricalImageMap(
    double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const
{
    double len;
    double theta;

    if ((image->getOnceFlag()) && ((y < 0.0) || (y > 1.0))) {
        return false;
    }
    *v = std::fmod(y * image->getYSize(), image->getYSize());

    len = java::Math::sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        return false;
    }
    x /= len;
    z /= len;

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
    theta /= 2.0 * java::Math::PI;

    *u = (theta * image->getXSize());
    return true;
}

bool
SolidTextureCoordinateMapper::torusImageMap(
    double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const
{
    double len;
    double phi;
    double theta;
    const double r0 = image->getImageGradient().x();

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

    x = len - r0;
    len = java::Math::sqrt(x * x + y * y);
    phi = java::Math::acos(-x / len);
    if (y > 0.0) {
        phi = 2.0 * java::Math::PI - phi;
    }

    theta /= 2.0 * java::Math::PI;
    phi /= 2.0 * java::Math::PI;
    *u = (-theta * image->getXSize());
    *v = (phi * image->getYSize());
    return true;
}

bool
SolidTextureCoordinateMapper::sphericalImageMap(
    double x, double y, double z, const RGBAImageHDRUncompressed *image, double *u, double *v) const
{
    double len;
    double phi;
    double theta;

    len = java::Math::sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        return false;
    }
    x /= len;
    y /= len;
    z /= len;

    phi = 0.5 + java::Math::asin(y) / java::Math::PI;

    len = java::Math::sqrt(x * x + z * z);
    if (len == 0.0) {
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
        theta /= 2.0 * java::Math::PI;
    }
    *u = (theta * image->getXSize());
    *v = (phi * image->getYSize());
    return true;
}

bool
SolidTextureCoordinateMapper::planarImageMap(
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

bool
SolidTextureCoordinateMapper::map(double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image,
    double *xCoordinate, double *yCoordinate, double smallTolerance) const
{
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

    *yCoordinate += smallTolerance;
    *xCoordinate += smallTolerance;
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
