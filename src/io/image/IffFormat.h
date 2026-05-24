#ifndef __IFF_FORMAT_H__
#define __IFF_FORMAT_H__

#include "environment/geometry/GeometryOperations.h"
#include "io/image/ChunkHeader.h"
#include "java/io/FileInputStream.h"
class RGBAPixel;

class IffFormat {
  public:
    static void iffError(void);
    static int readByte(java::FileInputStream &is);
    static int readWord(java::FileInputStream &is);
    static long readLong(java::FileInputStream &is);
    static void readChunkHeader(java::FileInputStream &is, ChunkHeader *dest);
    static void readIffImage(RGBAImage *image, char *filename);

  private:
    static RGBAPixel *sIffColourMap;
    static int sColourMapSize;
    static ChunkHeader sGlobalChunkHeader;
};

#endif
