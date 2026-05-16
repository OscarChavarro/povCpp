#ifndef __DUMP_H__
#define __DUMP_H__

#include "geom/geometry.h"

#define READ_MODE 0
#define WRITE_MODE 1
#define APPEND_MODE 2

class FileHandle {
  public:
    char *filename;
    int mode;
    int width, height;
    int buffer_size;
    char *buffer;
    FILE *file;
    char *(*Default_File_Name_p)(void);
    int (*Open_File_p)(FileHandle *handle, char *name, int *width, int *height,
        int buffer_size, int mode);
    void (*Write_Line_p)(
        FileHandle *handle, RGBAColor *line_data, int line_number);
    int (*Read_Line_p)(
        FileHandle *handle, RGBAColor *line_data, int *line_number);
    void (*Read_Image_p)(RGBAImage *Image, char *filename);
    void (*Close_File_p)(FileHandle *handle);
};

#define Default_File_Name(h) ((*((h)->Default_File_Name_p))())
#define Open_File(h, n, wd, ht, sz, m)                                         \
    ((*((h)->Open_File_p))(h, n, wd, ht, sz, m))
#define Write_Line(h, l, n) ((*((h)->Write_Line_p))(h, l, n))
#define Read_Line(h, l, n) ((*((h)->Read_Line_p))(h, l, n))
#define Read_Image(h, i) ((*((h)->Read_Image_p))(h, i))
#define Close_File(h) ((*((h)->Close_File_p))(h))

extern FileHandle *Get_Dump_File_Handle(void);
extern char *Default_Dump_File_Name(void);
extern int Open_Dump_File(FileHandle *handle, char *name, int *width,
    int *height, int buffer_size, int mode);
void Write_Dump_Line(FileHandle *handle, RGBAColor *line_data, int line_number);
extern int Read_Dump_Line(
    FileHandle *handle, RGBAColor *line_data, int *line_number);
extern int Read_Dump_Int_Line(
    FileHandle *handle, ImageLine *line_data, int *line_number);
extern void Read_Dump_Image(RGBAImage *Image, char *filename);
extern void Close_Dump_File(FileHandle *handle);

#endif
