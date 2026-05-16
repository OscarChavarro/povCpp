#ifndef __RAW_H__
#define __RAW_H__

#include "io/Dump.h"

extern FileHandle *getRawFileHandle(void);
extern char *defaultRawFileName(void);
extern int openRawFile(FileHandle *handle, char *name, int *width,
    int *height, int buffer_size, int mode);
void writeRawLine(FileHandle *handle, RGBAColor *line_data, int line_number);
extern int readRawLine(
    FileHandle *handle, RGBAColor *line_data, int *line_number);
extern void closeRawFile(FileHandle *handle);

#endif
