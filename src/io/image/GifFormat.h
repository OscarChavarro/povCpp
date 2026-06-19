#ifndef __GIF_FORMAT__
#define __GIF_FORMAT__

#include "java/io/FileInputStream.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"

class FileLocator;

class GifFormat {
  public:
    static void readGifImage(IndexedColorImageHDRUncompressed *image, const char *filename,
        const FileLocator &locator);
};

#endif
