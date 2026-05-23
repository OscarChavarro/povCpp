#ifndef __RAW_FORMAT_H__
#define __RAW_FORMAT_H__

#include "io/image/DumpFormat.h"

static constexpr const char *RED_RAW_FILE_EXTENSION = ".red";
static constexpr const char *GREEN_RAW_FILE_EXTENSION = ".grn";
static constexpr const char *BLUE_RAW_FILE_EXTENSION = ".blu";

class RawFormat {
  public:
    static FileInputStream *getRawFileInputStream(void);
    static char *defaultRawFileName(void);
    static int openRawFile(FileInputStream *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeRawLine(
        FileInputStream *handle, RGBAColor *lineData, int lineNumber);
    static int readRawLine(
        FileInputStream *handle, RGBAColor *lineData, int *lineNumber);
    static void closeRawFile(FileInputStream *handle);
};

#endif
