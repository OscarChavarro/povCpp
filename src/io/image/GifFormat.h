#ifndef __GIF_FORMAT_H__
#define __GIF_FORMAT_H__

#include "media/RGBAImage.h"
#include "media/RGBAPixel.h"
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
    static RGBAPixel16Bits *gifColourMap;
    static int colourmapSize;
};

#endif
