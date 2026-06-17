#ifndef __IMAGE_TEXTURE_H__
#define __IMAGE_TEXTURE_H__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
class ImageTexture {
  private:
    void noInterpolation(const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index) const;
    double biLinear(const double *corners, double x, double y) const;
    double normDist(const double *corners, double x, double y) const;
    void interp(const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index) const;
    void imageColorAt(const ControlledRGBAImageHDRUncompressed *image, double xCoordinate, double yCoordinate,
        ColorRgba *color, int *index) const;
    double pythagoreanSq(double a, double b) const;

  public:
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
};

#endif
