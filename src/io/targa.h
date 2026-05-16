#ifndef __TARGA_H__
#define __TARGA_H__

#include "io/dump.h"

extern FileHandle *Get_Targa_File_Handle(void);
extern char *Default_Targa_File_Name(void);
extern int Open_Targa_File(FileHandle *handle, char *name, int *width, int *height, int buffer_size, int mode);
void Write_Targa_Line(FileHandle *handle, RGBAColor *line_data, int line_number);
extern int Read_Targa_Line(FileHandle *handle, RGBAColor *line_data, int *line_number);
extern void Read_Targa_Image(RGBAImage *Image, char *filename);
extern void Close_Targa_File(FileHandle *handle);
extern int Read_Targa_Int_Line(FileHandle *handle,ImageLine *line_data);

#endif
