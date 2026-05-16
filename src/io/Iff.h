#ifndef __IFF_H__
#define __IFF_H__

#include "geom/Geometry.h"
#include "io/ChunkHeader.h"

extern void iff_error(void);
extern int read_byte(FILE *f);
extern int read_word(FILE *f);
extern long read_long(FILE *f);
extern void Read_Chunk_Header(FILE *f, ChunkHeader *dest);
extern void Read_Iff_Image(RGBAImage *Image, char *filename);

#endif
