#ifndef __TXTMAP_H__
#define __TXTMAP_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/solidTexture/Texture.h"
#include "media/solidTexture/TextureImage.h"

class mapTextureFixture {
  public:
    int map(double x, double y, double z, Texture *texture,
        textureImage *image, double *xcoor, double *ycoor,
        double smallTolerance);
    void imageMap(
        double x, double y, double z, Texture *texture, RGBAColor *color, double smallTolerance);
    Texture *materialMap(
        Vector3Dd *intersectionPoint, Texture *texture,
        double smallTolerance);
    void bumpMap(
        double x, double y, double z, Texture *texture, Vector3Dd *normal, double smallTolerance);

  private:
    int cylindricalImageMap(
        double x, double y, double z, textureImage *image, double *u, double *v);
    int torusImageMap(
        double x, double y, double z, textureImage *image, double *u, double *v);
    int sphericalImageMap(
        double x, double y, double z, RGBAImageHDRUncompressed *image, double *u, double *v);
    int planarImageMap(
        double x, double y, double z, textureImage *image, double *u, double *v);
    void noInterpolation(textureImage *image, double xcoor, double ycoor,
        RGBAColor *color, int *index);
    double bilinear(double *corners, double x, double y);
    double normDist(double *corners, double x, double y);
    void interp(textureImage *image, double xcoor, double ycoor,
        RGBAColor *color, int *index);
    void imageColorAt(textureImage *image, double xcoor, double ycoor,
        RGBAColor *color, int *index);
    double pythagoreanSq(double a, double b);
};

#endif
