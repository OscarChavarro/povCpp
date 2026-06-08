#ifndef __TXTMAP_H__
#define __TXTMAP_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/Texture.h"
#include "media/TextureImage.h"

class MapTextureFixture {
  public:
    static int map(double x, double y, double z, Texture *texture,
        TextureImage *image, double *xcoor, double *ycoor,
        double smallTolerance);
    static void imageMap(
        double x, double y, double z, Texture *texture, RGBAColor *colour,
        int debugEnabled, double smallTolerance);
    static Texture *materialMap(
        Vector3Dd *intersectionPoint, Texture *texture, int debugEnabled,
        double smallTolerance);
    static void bumpMap(
        double x, double y, double z, Texture *texture, Vector3Dd *normal,
        int debugEnabled, double smallTolerance);

  private:
    static int cylindricalImageMap(
        double x, double y, double z, TextureImage *image, double *u, double *v);
    static int torusImageMap(
        double x, double y, double z, TextureImage *image, double *u, double *v);
    static int sphericalImageMap(
        double x, double y, double z, RGBAImage *image, double *u, double *v);
    static int planarImageMap(
        double x, double y, double z, TextureImage *image, double *u, double *v);
    static void noInterpolation(RGBAImage *image, double xcoor, double ycoor,
        RGBAColor *colour, int *index);
    static double bilinear(double *corners, double x, double y);
    static double normDist(double *corners, double x, double y);
    static void interp(TextureImage *image, double xcoor, double ycoor,
        RGBAColor *colour, int *index);
    static void imageColourAt(TextureImage *image, double xcoor, double ycoor,
        RGBAColor *colour, int *index);
    static inline double pythagoreanSq(double a, double b);
};

#endif
