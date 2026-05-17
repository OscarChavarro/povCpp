#ifndef __DUMP_FORMAT_H__
#define __DUMP_FORMAT_H__

#include "io/FileHandle.h"

static constexpr int READ_MODE = 0;
static constexpr int WRITE_MODE = 1;
static constexpr int APPEND_MODE = 2;

inline char *defaultFileName(FileHandle *h)
{
    return ((*((h)->Default_File_Name_p))());
}

inline int openFile(
    FileHandle *h, char *n, int *wd, int *ht, int sz, int m)
{
    return ((*((h)->Open_File_p))(h, n, wd, ht, sz, m));
}

class DumpFormat {
  public:
    static FileHandle *getDumpFileHandle(void);
    static char *defaultDumpFileName(void);
    static int openDumpFile(FileHandle *handle, char *name, int *width,
        int *height, int bufferSize, int mode);
    static void writeDumpLine(
        FileHandle *handle, RGBAColor *lineData, int lineNumber);
    static int readDumpLine(
        FileHandle *handle, RGBAColor *lineData, int *lineNumber);
    static int readDumpIntLine(
        FileHandle *handle, ImageLine *lineData, int *lineNumber);
    static void readDumpImage(RGBAImage *image, char *name);
    static void closeDumpFile(FileHandle *handle);
    static inline void writeLine(FileHandle *h, RGBAColor *l, int n)
    {
        ((*((h)->Write_Line_p))(h, l, n));
    }
    static inline int readLine(FileHandle *h, RGBAColor *l, int *n)
    {
        return ((*((h)->Read_Line_p))(h, l, n));
    }
    static inline void readImage(FileHandle *h, RGBAImage *i)
    {
        (((*((h)->Read_Image_p))(i, (h)->filename)));
    }
    static inline void closeFile(FileHandle *h)
    {
        ((*((h)->Close_File_p))(h));
    }
};

#endif
