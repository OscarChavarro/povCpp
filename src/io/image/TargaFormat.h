#ifndef __TARGA_FORMAT_H__
#define __TARGA_FORMAT_H__

#include "io/image/ImageFileHandle.h"
#include "media/ImageData.h"

class RGBAColor;
class RGBAImage;

class TargaFormat {
  public:
    static ImageFileHandle *getTargaFileInputStream(void);
    static char *defaultTargaFileName(void);
    static int openTargaFile(ImageFileHandle *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeTargaLine(
        ImageFileHandle *handle, RGBAColor *lineData, int lineNumber);
    static int readTargaLine(
        ImageFileHandle *handle, RGBAColor *lineData, int *lineNumber);
    static void readTargaImage(RGBAImage *image, char *name);
    static void closeTargaFile(ImageFileHandle *handle);
    static int readTargaIntLine(ImageFileHandle *handle, ImageLine *lineData);
};

#endif
