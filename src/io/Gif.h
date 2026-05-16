#ifndef __GIF_H__
#define __GIF_H__

#include "geom/Geometry.h"

extern int outLine(unsigned char *pixels, int linelen);
extern int getByte(void);
extern void readGifImage(RGBAImage *Image, char *filename);

#endif
