#ifndef __JAVA_IO_FILE_INPUT_STREAM_H__
#define __JAVA_IO_FILE_INPUT_STREAM_H__

#include <cstdio>

#include "java/io/FileConstants.h"

class RGBAColor;
class RGBAImage;

class FileInputStream {
  public:
    static constexpr int READ_MODE = 0;
    static constexpr int WRITE_MODE = 1;
    static constexpr int APPEND_MODE = 2;

    char *filename;
    int mode;
    int width, height;
    int buffer_size;
    char *buffer;
    FILE *file;
    char *(*Default_File_Name_p)(void);
    int (*Open_File_p)(FileInputStream *handle, char *name, int *width, int *height,
        int buffer_size, int mode);
    void (*Write_Line_p)(
        FileInputStream *handle, RGBAColor *line_data, int line_number);
    int (*Read_Line_p)(
        FileInputStream *handle, RGBAColor *line_data, int *line_number);
    void (*Read_Image_p)(RGBAImage *image, char *filename);
    void (*Close_File_p)(FileInputStream *handle);

    char *defaultFileName()
    {
        return Default_File_Name_p();
    }

    int open(char *name, int *imageWidth, int *imageHeight, int bufferSize,
        int openMode)
    {
        return Open_File_p(
            this, name, imageWidth, imageHeight, bufferSize, openMode);
    }

    void writeLine(RGBAColor *lineData, int lineNumber)
    {
        Write_Line_p(this, lineData, lineNumber);
    }

    int readLine(RGBAColor *lineData, int *lineNumber)
    {
        return Read_Line_p(this, lineData, lineNumber);
    }

    void close()
    {
        Close_File_p(this);
    }
};

#endif
