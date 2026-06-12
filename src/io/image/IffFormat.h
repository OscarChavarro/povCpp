#ifndef __IFF_FORMAT_H__
#define __IFF_FORMAT_H__

#include "java/io/FileInputStream.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "io/image/ChunkHeader.h"

class IffFormat {
  public:
    static void iffError(void);
    static int readByte(java::FileInputStream &is);
    static int readWord(java::FileInputStream &is);
    static long readLong(java::FileInputStream &is);
    static void readChunkHeader(java::FileInputStream &is, ChunkHeader *dest);

    // Returns a new IndexedColorImageHDRUncompressed* if the file is paletted; fills directOut and
    // returns nullptr if the file is direct-color (HAM or 24-plane).
    static IndexedColorImageHDRUncompressed *readIffImage(RGBAImageHDRUncompressed *directOut, const char *filename);

  private:
    static RGBAPixelHDR *sIffColorMap;
    static int sColorMapSize;
    static ChunkHeader sGlobalChunkHeader;
};

#endif
