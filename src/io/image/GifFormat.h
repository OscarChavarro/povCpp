#ifndef __GIF_FORMAT_H__
#define __GIF_FORMAT_H__

#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "java/io/FileInputStream.h"

class GifFormat {
  public:
    static int outLine(unsigned char *pixels, int linelen);
    static int getByte(void);
    static void readGifImage(IndexedColorImageHDRUncompressed *image, char *filename);

  private:
    static IndexedColorImageHDRUncompressed *currentImage;
    static int bitmapLine;
    static java::FileInputStream *bitStream;
    static RGBAPixelHDR *gifColorMap;
    static int colorMapSize;
};

#endif
