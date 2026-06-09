#ifndef __GIF_FORMAT_H__
#define __GIF_FORMAT_H__

#include "media/IndexedImage.h"
#include "java/io/FileInputStream.h"

class GifFormat {
  public:
    static int outLine(unsigned char *pixels, int linelen);
    static int getByte(void);
    static void readGifImage(IndexedImage *image, char *filename);

  private:
    static IndexedImage *currentImage;
    static int bitmapLine;
    static java::FileInputStream *bitStream;
    static RGBAPixelHDR *gifColourMap;
    static int colourmapSize;
};

#endif
