#ifndef __IFF_H__
#define __IFF_H__

#include "geom/Geometry.h"
#include "io/ChunkHeader.h"

extern void iffError(void);
extern int readByte(FILE *f);
extern int readWord(FILE *f);
extern long readLong(FILE *f);
extern void readChunkHeader(FILE *f, ChunkHeader *dest);
extern void readIffImage(RGBAImage *Image, char *filename);

#endif
