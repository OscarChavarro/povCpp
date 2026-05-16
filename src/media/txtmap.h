#ifndef __TXTMAP_H__
#define __TXTMAP_H__

#include "common/frame.h"
#include "common/vector.h"
#include "media/texture.h"

extern int map(DBL x, DBL y, DBL z, Texture *Texture, RGBAImage *Image,
    DBL *xcoor, DBL *ycoor);
extern void image_map(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern Texture *material_map(Vector3D *Intersection_Point, Texture *Texture);
extern void bump_map(
    DBL x, DBL y, DBL z, Texture *Texture, Vector3D *normal); /* CdW 7/8/91*/

#endif
