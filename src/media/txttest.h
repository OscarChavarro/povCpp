#ifndef __TXTTEST_H__
#define __TXTTEST_H__

#include "common/frame.h"
#include "common/vector.h"
#include "media/texture.h"

extern void painted1(
    DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour); /* CdW 7/2/91 */
extern void painted2(
    DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour); /* CdW 7/2/91 */
extern void painted3(
    DBL x, DBL y, DBL z, Texture *Texture, RGBAColor *colour); /* CdW 7/2/91 */
extern void bumpy1(
    DBL x, DBL y, DBL z, Texture *Texture, Vector3D *normal); /* CdW 7/2/91*/
extern void bumpy2(
    DBL x, DBL y, DBL z, Texture *Texture, Vector3D *normal); /* CdW 7/2/91*/
extern void bumpy3(
    DBL x, DBL y, DBL z, Texture *Texture, Vector3D *normal); /* CdW 7/2/91*/

#endif
