#ifndef __DUMP_FORMAT_H__
#define __DUMP_FORMAT_H__

#include "java/io/FileInputStream.h"

class ImageLine;
class RGBAColor;
class RGBAImage;

static constexpr int READ_MODE = 0;
static constexpr int WRITE_MODE = 1;
static constexpr int APPEND_MODE = 2;

inline char *
defaultFileName(FileInputStream *h)
{
    return ((*((h)->Default_File_Name_p))());
}

inline int
openFile(FileInputStream *h, char *n, int *wd, int *ht, int sz, int m)
{
    return ((*((h)->Open_File_p))(h, n, wd, ht, sz, m));
}

class DumpFormat {
  public:
    static FileInputStream *getDumpFileInputStream(void);
    static char *defaultDumpFileName(void);
    static int openDumpFile(FileInputStream *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeDumpLine(
        FileInputStream *handle, RGBAColor *lineData, int lineNumber);
    static int readDumpLine(
        FileInputStream *handle, RGBAColor *lineData, int *lineNumber);
    static int readDumpIntLine(
        FileInputStream *handle, ImageLine *lineData, int *lineNumber);
    static void readDumpImage(RGBAImage *image, char *name);
    static void closeDumpFile(FileInputStream *handle);
    static inline void
    writeLine(FileInputStream *h, RGBAColor *l, int n)
    {
        ((*((h)->Write_Line_p))(h, l, n));
    }
    static inline int
    readLine(FileInputStream *h, RGBAColor *l, int *n)
    {
        return ((*((h)->Read_Line_p))(h, l, n));
    }
    static inline void
    readImage(FileInputStream *h, RGBAImage *i)
    {
        (((*((h)->Read_Image_p))(i, (h)->filename)));
    }
    static inline void
    closeFile(FileInputStream *h)
    {
        ((*((h)->Close_File_p))(h));
    }
};

#endif
