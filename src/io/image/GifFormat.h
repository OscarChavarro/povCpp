#ifndef __GIF_FORMAT__
#define __GIF_FORMAT__

#include "java/io/FileInputStream.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"

class GifFormat {
  public:
    static void readGifImage(IndexedColorImageHDRUncompressed *image, const char *filename);
};

#endif
