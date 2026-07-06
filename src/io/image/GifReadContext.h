#ifndef __GIF_READ_CONTEXT__
#define __GIF_READ_CONTEXT__

#include "java/io/FileInputStream.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"

// Scratch state threaded through the GifDecoder C-style callbacks
// (GifFormat::outLine/getByte) via a void* context pointer.
class GifReadContext {
  public:
    GifReadContext(
        IndexedColorImageHDRUncompressed *currentImage,
        int bitmapLine,
        java::FileInputStream *bitStream,
        RGBAPixelHDR *gifColorMap,
        int colorMapSize) :
        currentImage(currentImage),
        bitmapLine(bitmapLine),
        bitStream(bitStream),
        gifColorMap(gifColorMap),
        colorMapSize(colorMapSize)
    {}

    IndexedColorImageHDRUncompressed *getCurrentImage() const { return currentImage; }
    int getBitmapLine() const { return bitmapLine; }
    java::FileInputStream *getBitStream() const { return bitStream; }
    RGBAPixelHDR *getGifColorMap() const { return gifColorMap; }
    int getColorMapSize() const { return colorMapSize; }

    void setBitmapLine(int value) { bitmapLine = value; }
    void incrementBitmapLine() { bitmapLine++; }
    void setBitStream(java::FileInputStream *value) { bitStream = value; }
    void setGifColorMap(RGBAPixelHDR *value) { gifColorMap = value; }
    void setColorMapSize(int value) { colorMapSize = value; }

  private:
    IndexedColorImageHDRUncompressed *currentImage;
    int bitmapLine;
    java::FileInputStream *bitStream;
    RGBAPixelHDR *gifColorMap;
    int colorMapSize;
};

#endif
