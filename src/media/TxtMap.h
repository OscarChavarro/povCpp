#ifndef __TXTMAP_H__
#define __TXTMAP_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "media/Texture.h"

extern int map(DBL x, DBL y, DBL z, Texture *Texture, RGBAImage *Image,
    DBL *xcoor, DBL *ycoor);
extern void imageMap(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern Texture *materialMap(Vector3D *Intersection_Point, Texture *Texture);
extern void bumpMap(
    DBL x, DBL y, DBL z, Texture *Texture, Vector3D *normal); /* CdW 7/8/91*/

#endif
