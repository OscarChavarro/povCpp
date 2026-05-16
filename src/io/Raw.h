#ifndef __RAW_H__
#define __RAW_H__

#include "io/Dump.h"

extern FileHandle *Get_Raw_File_Handle(void);
extern char *Default_Raw_File_Name(void);
extern int Open_Raw_File(FileHandle *handle, char *name, int *width,
    int *height, int buffer_size, int mode);
void Write_Raw_Line(FileHandle *handle, RGBAColor *line_data, int line_number);
extern int Read_Raw_Line(
    FileHandle *handle, RGBAColor *line_data, int *line_number);
extern void Close_Raw_File(FileHandle *handle);

#endif
