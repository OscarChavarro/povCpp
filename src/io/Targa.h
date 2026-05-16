#ifndef __TARGA_H__
#define __TARGA_H__

#include "io/Dump.h"

extern FileHandle *getTargaFileHandle(void);
extern char *defaultTargaFileName(void);
extern int openTargaFile(FileHandle *handle, char *name, int *width,
    int *height, int buffer_size, int mode);
void writeTargaLine(
    FileHandle *handle, RGBAColor *line_data, int line_number);
extern int readTargaLine(
    FileHandle *handle, RGBAColor *line_data, int *line_number);
extern void readTargaImage(RGBAImage *Image, char *filename);
extern void closeTargaFile(FileHandle *handle);
extern int readTargaIntLine(FileHandle *handle, ImageLine *line_data);

#endif
