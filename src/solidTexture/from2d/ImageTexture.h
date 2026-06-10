#ifndef __TXTMAP_H__
#define __TXTMAP_H__

#include "solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class ImageTexture {
  public:
    int map(
        double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, double *xCoordinate,
        double *yCoordinate, double smallTolerance);
    void imageMap(
        double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, ColorRgba *color,
        double smallTolerance);
    int materialMap(
        Vector3Dd *intersectionPoint, Matrix4x4d *textureTransformationInverse,
        ControlledRGBAImageHDRUncompressed *materialImage, int numberOfMaterials,
        double smallTolerance);
    void bumpMap(
        double x, double y, double z, ControlledRGBAImageHDRUncompressed *bumpImage,
        double bumpAmount, Vector3Dd *normal, double smallTolerance);

  private:
    int cylindricalImageMap(
        double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, double *u, double *v);
    int torusImageMap(
        double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, double *u, double *v);
    int sphericalImageMap(
        double x, double y, double z, RGBAImageHDRUncompressed *image, double *u, double *v);
    int planarImageMap(
        double x, double y, double z, ControlledRGBAImageHDRUncompressed *image, double *u, double *v);
    void noInterpolation(ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index);
    double biLinear(double *corners, double x, double y);
    double normDist(double *corners, double x, double y);
    void interp(ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index);
    void imageColorAt(ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index);
    double pythagoreanSq(double a, double b);
};

#endif
