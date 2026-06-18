#ifndef __GIF_FORMAT__
#define __GIF_FORMAT__

#include "java/io/FileInputStream.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"

class GifFormat {
  public:
    static int outLine(const unsigned char *pixels, int linelen);
    static int getByte(void);
    static void readGifImage(IndexedColorImageHDRUncompressed *image, const char *filename);

  private:
    static IndexedColorImageHDRUncompressed *currentImage;
    static int bitmapLine;
    static java::FileInputStream *bitStream;
    static RGBAPixelHDR *gifColorMap;
    static int colorMapSize;
};

#endif
