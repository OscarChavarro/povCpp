#ifndef __RAW_FORMAT_H__
#define __RAW_FORMAT_H__

#include "io/image/ImageFileHandle.h"

static constexpr const char *RED_RAW_FILE_EXTENSION = ".red";
static constexpr const char *GREEN_RAW_FILE_EXTENSION = ".grn";
static constexpr const char *BLUE_RAW_FILE_EXTENSION = ".blu";

class RGBAColor;

class RawFormat {
  public:
    static ImageFileHandle *getRawFileInputStream(void);
    static char *defaultRawFileName(void);
    static int openRawFile(ImageFileHandle *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeRawLine(
        ImageFileHandle *handle, RGBAColor *lineData, int lineNumber);
    static int readRawLine(
        ImageFileHandle *handle, RGBAColor *lineData, int *lineNumber);
    static void closeRawFile(ImageFileHandle *handle);
};

#endif
