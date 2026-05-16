#ifndef __TXTCOLOR_H__
#define __TXTCOLOR_H__

#include "common/Frame.h"
#include "common/Vector.h"
#include "media/Texture.h"

extern void Colour_At(
    RGBAColor *Colour, Texture *Texture, Vector3D *Intersection_Point);
extern void agate(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void bozo(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void brick(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void checker(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void checker_texture(
    DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void gradient(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void granite(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void marble(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void spotted(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
extern void wood(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);
/* Two new textures from Scott Taylor ONION & LEOPARD 7/18/91*/
extern void leopard(
    DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour); /* SWT 7/18/91 */
extern void onion(DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour);

#endif
