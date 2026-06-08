#ifndef __IFF_FORMAT_H__
#define __IFF_FORMAT_H__

#include "media/RGBAImage.h"
#include "media/IndexedImage.h"
#include "io/image/ChunkHeader.h"
#include "java/io/FileInputStream.h"

class IffFormat {
  public:
    static void iffError(void);
    static int readByte(java::FileInputStream &is);
    static int readWord(java::FileInputStream &is);
    static long readLong(java::FileInputStream &is);
    static void readChunkHeader(java::FileInputStream &is, ChunkHeader *dest);

    // Returns a new IndexedImage* if the file is paletted; fills directOut and
    // returns nullptr if the file is direct-color (HAM or 24-plane).
    static IndexedImage *readIffImage(RGBAImage *directOut, char *filename);

  private:
    static RGBAPixel16Bits *sIffColourMap;
    static int sColourMapSize;
    static ChunkHeader sGlobalChunkHeader;
};

#endif
