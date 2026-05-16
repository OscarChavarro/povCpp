#ifndef __DUMP_H__
#define __DUMP_H__

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

inline void writeLine(FileHandle *h, RGBAColor *l, int n)
{
    ((*((h)->Write_Line_p))(h, l, n));
}

inline int readLine(FileHandle *h, RGBAColor *l, int *n)
{
    return ((*((h)->Read_Line_p))(h, l, n));
}

inline void readImage(FileHandle *h, RGBAImage *i)
{
    (((*((h)->Read_Image_p))(i, (h)->filename)));
}

inline void closeFile(FileHandle *h)
{
    ((*((h)->Close_File_p))(h));
}

extern FileHandle *getDumpFileHandle(void);
extern char *defaultDumpFileName(void);
extern int openDumpFile(FileHandle *handle, char *name, int *width,
    int *height, int buffer_size, int mode);
void writeDumpLine(FileHandle *handle, RGBAColor *line_data, int line_number);
extern int readDumpLine(
    FileHandle *handle, RGBAColor *line_data, int *line_number);
extern int readDumpIntLine(
    FileHandle *handle, ImageLine *line_data, int *line_number);
extern void readDumpImage(RGBAImage *Image, char *filename);
extern void closeDumpFile(FileHandle *handle);

#endif
