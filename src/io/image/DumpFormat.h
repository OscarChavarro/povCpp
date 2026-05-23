#ifndef __DUMP_FORMAT_H__
#define __DUMP_FORMAT_H__

#include "io/image/ImageFileHandle.h"
#include "media/ImageData.h"

class RGBAColor;
class RGBAImage;

class DumpFormat {
  public:
    static ImageFileHandle *getDumpFileInputStream(void);
    static char *defaultDumpFileName(void);
    static int openDumpFile(ImageFileHandle *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeDumpLine(
        ImageFileHandle *handle, RGBAColor *lineData, int lineNumber);
    static int readDumpLine(
        ImageFileHandle *handle, RGBAColor *lineData, int *lineNumber);
    static int readDumpIntLine(
        ImageFileHandle *handle, ImageLine *lineData, int *lineNumber);
    static void closeDumpFile(ImageFileHandle *handle);
    static void readDumpImage(RGBAImage *image, char *name);
};

#endif
