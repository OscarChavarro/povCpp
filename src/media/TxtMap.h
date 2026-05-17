#ifndef __TXTMAP_H__
#define __TXTMAP_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "media/Texture.h"

class MapTextures {
  public:
    static int map(double x, double y, double z, Texture *texture, RGBAImage *image,
        double *xcoor, double *ycoor);
    static void imageMap(
        double x, double y, double z, Texture *texture, RGBAColor *colour);
    static Texture *materialMap(
        Vector3D *intersectionPoint, Texture *texture);
    static void bumpMap(
        double x, double y, double z, Texture *texture, Vector3D *normal);

  private:
    static int cylindricalImageMap(
        double x, double y, double z, RGBAImage *image, double *u, double *v);
    static int torusImageMap(
        double x, double y, double z, RGBAImage *image, double *u, double *v);
    static int sphericalImageMap(
        double x, double y, double z, RGBAImage *image, double *u, double *v);
    static int planarImageMap(
        double x, double y, double z, RGBAImage *image, double *u, double *v);
    static void noInterpolation(
        RGBAImage *image, double xcoor, double ycoor, RGBAColor *colour, int *index);
    static double bilinear(double *corners, double x, double y);
    static double normDist(double *corners, double x, double y);
    static void interp(
        RGBAImage *image, double xcoor, double ycoor, RGBAColor *colour, int *index);
    static void imageColourAt(
        RGBAImage *image, double xcoor, double ycoor, RGBAColor *colour, int *index);
    static inline double pythagoreanSq(double a, double b);
};

#endif
