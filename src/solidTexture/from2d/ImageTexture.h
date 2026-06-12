#ifndef __TXTMAP_H__
#define __TXTMAP_H__

#include "solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class ImageTexture {
  public:
    int map(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *xCoordinate,
        double *yCoordinate, double smallTolerance) const;
    void imageMap(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, ColorRgba *color,
        double smallTolerance) const;
    int materialMap(
        const Vector3Dd *intersectionPoint, const Matrix4x4d *textureTransformationInverse,
        const ControlledRGBAImageHDRUncompressed *materialImage, int numberOfMaterials,
        double smallTolerance) const;
    void bumpMap(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *bumpImage,
        double bumpAmount, Vector3Dd *normal, double smallTolerance) const;

  private:
    int cylindricalImageMap(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const;
    int torusImageMap(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const;
    int sphericalImageMap(
        double x, double y, double z, const RGBAImageHDRUncompressed *image, double *u, double *v) const;
    int planarImageMap(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const;
    void noInterpolation(const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index) const;
    double biLinear(const double *corners, double x, double y) const;
    double normDist(const double *corners, double x, double y) const;
    void interp(const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index) const;
    void imageColorAt(const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index) const;
    double pythagoreanSq(double a, double b) const;
};

#endif
