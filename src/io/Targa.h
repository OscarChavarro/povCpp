#ifndef __TARGA_H__
#define __TARGA_H__

#include "io/Dump.h"

class TargaFormat {
  public:
    static FileHandle *getTargaFileHandle(void);
    static char *defaultTargaFileName(void);
    static int openTargaFile(FileHandle *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeTargaLine(
        FileHandle *handle, RGBAColor *lineData, int lineNumber);
    static int readTargaLine(
        FileHandle *handle, RGBAColor *lineData, int *lineNumber);
    static void readTargaImage(RGBAImage *image, char *name);
    static void closeTargaFile(FileHandle *handle);
    static int readTargaIntLine(FileHandle *handle, ImageLine *lineData);
};

#endif
