#ifndef __GIF_H__
#define __GIF_H__

#include "geom/geometry.h"

extern int out_line(unsigned char *pixels, int linelen);
extern int get_byte(void);
extern void Read_Gif_Image(RGBAImage *Image, char *filename);

#endif
