#ifndef __TXTMAP_H__
#define __TXTMAP_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "media/Texture.h"

class MapTextures {
  public:
    static int map(DBL x, DBL y, DBL z, Texture *texture, RGBAImage *image,
        DBL *xcoor, DBL *ycoor);
    static void imageMap(
        DBL x, DBL y, DBL z, Texture *texture, RGBAColor *colour);
    static Texture *materialMap(
        Vector3D *intersectionPoint, Texture *texture);
    static void bumpMap(
        DBL x, DBL y, DBL z, Texture *texture, Vector3D *normal);

  private:
    static int cylindricalImageMap(
        DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v);
    static int torusImageMap(
        DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v);
    static int sphericalImageMap(
        DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v);
    static int planarImageMap(
        DBL x, DBL y, DBL z, RGBAImage *image, DBL *u, DBL *v);
    static void noInterpolation(
        RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index);
    static DBL bilinear(DBL *corners, DBL x, DBL y);
    static DBL normDist(DBL *corners, DBL x, DBL y);
    static void interp(
        RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index);
    static void imageColourAt(
        RGBAImage *image, DBL xcoor, DBL ycoor, RGBAColor *colour, int *index);
};

#endif
