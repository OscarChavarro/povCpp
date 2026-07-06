#ifndef __GIF_FORMAT__
#define __GIF_FORMAT__

#include "java/io/FileInputStream.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/io/FileLocator.h"
#include "io/image/GifReadContext.h"

class GifFormat {
  public:
    static void readGifImage(IndexedColorImageHDRUncompressed *image, const char *filename,
        const FileLocator &locator);

  private:
    // GifDecoder::decoder() calls these back through plain function
    // pointers (GifInputContext), so they must stay static (no implicit
    // this) - their signature is otherwise identical to a free function.
    static int outLine(void *context, const unsigned char *pixels, int linelen);
    static int getByte(void *context);
};

#endif
