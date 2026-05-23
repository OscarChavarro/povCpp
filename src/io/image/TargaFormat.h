#ifndef __TARGA_FORMAT_H__
#define __TARGA_FORMAT_H__

#include "io/image/DumpFormat.h"

class TargaFormat {
  public:
    static FileInputStream *getTargaFileInputStream(void);
    static char *defaultTargaFileName(void);
    static int openTargaFile(FileInputStream *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeTargaLine(
        FileInputStream *handle, RGBAColor *lineData, int lineNumber);
    static int readTargaLine(
        FileInputStream *handle, RGBAColor *lineData, int *lineNumber);
    static void readTargaImage(RGBAImage *image, char *name);
    static void closeTargaFile(FileInputStream *handle);
    static int readTargaIntLine(FileInputStream *handle, ImageLine *lineData);
};

#endif
