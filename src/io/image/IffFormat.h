#ifndef __IFF_FORMAT_H__
#define __IFF_FORMAT_H__

#include "environment/geometry/GeometryOperations.h"
#include "io/image/ChunkHeader.h"

class IffFormat {
  public:
    static void iffError(void);
    static int readByte(FILE *f);
    static int readWord(FILE *f);
    static long readLong(FILE *f);
    static void readChunkHeader(FILE *f, ChunkHeader *dest);
    static void readIffImage(RGBAImage *image, char *filename);
};

#endif
