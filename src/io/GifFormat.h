#ifndef __GIF_FORMAT_H__
#define __GIF_FORMAT_H__

#include "geom/GeometryOps.h"

class GifFormat {
  public:
    static int outLine(unsigned char *pixels, int linelen);
    static int getByte(void);
    static void readGifImage(RGBAImage *image, char *filename);
};

#endif
