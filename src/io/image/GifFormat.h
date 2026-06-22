#ifndef __GIF_FORMAT__
#define __GIF_FORMAT__

#include "java/io/FileInputStream.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "io/binaryIo/FileLocator.h"

// Scratch state threaded through the GifDecoder C-style callbacks
// (GifFormat::outLine/getByte) via a void* context pointer.
struct GifReadContext {
    IndexedColorImageHDRUncompressed *currentImage;
    int bitmapLine;
    java::FileInputStream *bitStream;
    RGBAPixelHDR *gifColorMap;
    int colorMapSize;
};

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
