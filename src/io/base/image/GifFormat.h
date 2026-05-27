#ifndef __GIF_FORMAT_H__
#define __GIF_FORMAT_H__

#include "environment/geometry/GeometryOperations.h"
#include "java/io/FileInputStream.h"

class GifFormat {
  public:
    static int outLine(unsigned char *pixels, int linelen);
    static int getByte(void);
    static void readGifImage(RGBAImage *image, char *filename);

  private:
    static RGBAImage *currentImage;
    static int bitmapLine;
    static java::FileInputStream *bitStream;
    static RGBAPixel *gifColourMap;
    static int colourmapSize;
};

#endif
