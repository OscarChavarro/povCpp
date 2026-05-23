#ifndef __RAW_FORMAT_H__
#define __RAW_FORMAT_H__

#include "io/image/DumpFormat.h"

class RawFormat {
  public:
    static FileHandle *getRawFileHandle(void);
    static char *defaultRawFileName(void);
    static int openRawFile(FileHandle *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeRawLine(
        FileHandle *handle, RGBAColor *lineData, int lineNumber);
    static int readRawLine(
        FileHandle *handle, RGBAColor *lineData, int *lineNumber);
    static void closeRawFile(FileHandle *handle);
};

#endif
