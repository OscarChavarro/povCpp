#ifndef __DUMP_H__
#define __DUMP_H__

#include "io/FileHandle.h"

static constexpr int READ_MODE = 0;
static constexpr int WRITE_MODE = 1;
static constexpr int APPEND_MODE = 2;

#define defaultFileName(h) ((*((h)->Default_File_Name_p))())
#define openFile(h, n, wd, ht, sz, m)                                         \
    ((*((h)->Open_File_p))(h, n, wd, ht, sz, m))
#define writeLine(h, l, n) ((*((h)->Write_Line_p))(h, l, n))
#define readLine(h, l, n) ((*((h)->Read_Line_p))(h, l, n))
#define readImage(h, i) ((*((h)->Read_Image_p))(h, i))
#define closeFile(h) ((*((h)->Close_File_p))(h))

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
