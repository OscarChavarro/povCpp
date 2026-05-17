#ifndef __FILE_HANDLE_H__
#define __FILE_HANDLE_H__

#include "geom/GeometryOperations.h"

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

#endif
