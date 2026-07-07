#ifndef __SOLIDTEXTURECOORDINATEMAPPER__
#define __SOLIDTEXTURECOORDINATEMAPPER__

#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
class SolidTextureCoordinateMapper {
  private:
    bool cylindricalImageMap(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const;
    bool torusImageMap(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const;
    bool sphericalImageMap(
        double x, double y, double z, const RGBAImageHDRUncompressed *image, double *u, double *v) const;
    bool planarImageMap(
        double x, double y, double z, const ControlledRGBAImageHDRUncompressed *image, double *u, double *v) const;

  public:
    bool map(
        double x, double y, double z,
        const ControlledRGBAImageHDRUncompressed *image,
        double *xCoordinate, double *yCoordinate, double smallTolerance) const;
};

#endif
