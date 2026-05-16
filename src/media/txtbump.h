#ifndef __TXTBUMP_H__
#define __TXTBUMP_H__

#include "common/frame.h"
#include "common/vector.h"
#include "media/texture.h"

extern void bumps(DBL x, DBL y, DBL z, Texture *Texture, Vector3D *normal);
extern void dents(DBL x, DBL y, DBL z, Texture *Texture, Vector3D *normal);
extern void ripples(DBL x, DBL y, DBL z, Texture *Texture, Vector3D *Vector);
extern void waves(DBL x, DBL y, DBL z, Texture *Texture, Vector3D *Vector);
extern void wrinkles(DBL x, DBL y, DBL z, Texture *Texture, Vector3D *normal);

#endif
