/****************************************************************************
 *                     targa.c
 *
 *  This module contains the code to read and write the Targa output file
 *  format.
 *
 *****************************************************************************/

#include "io/TargaFormat.h"
#include "app/PovApp.h"
#include "common/FrameConfig.h"

int targaLineNumber = 0;

extern int firstLine;

FileHandle *
TargaFormat::getTargaFileHandle()
{
    FileHandle *handle;

    handle = new FileHandle;
    if (handle == nullptr) {
        fprintf(stderr, "Cannot allocate memory for output file handle\n");
        return (nullptr);
    }

    handle->Default_File_Name_p = TargaFormat::defaultTargaFileName;
    handle->Open_File_p = TargaFormat::openTargaFile;
    handle->Write_Line_p = TargaFormat::writeTargaLine;
    handle->Read_Line_p = TargaFormat::readTargaLine;
    handle->Read_Image_p = TargaFormat::readTargaImage;
    handle->Close_File_p = TargaFormat::closeTargaFile;
    handle->file = nullptr;
    handle->buffer_size = 0;
    handle->buffer = nullptr;
    return (handle);
}

static char defaultTargaFilename[] = "data.tga";

char *
TargaFormat::defaultTargaFileName()
{
    return defaultTargaFilename;
}

int
TargaFormat::openTargaFile(FileHandle *handle, char *name, int *width,
    int *height, int bufferSize, int mode)
{
    int data1;
    int data2;
    int i;

    handle->mode = mode;
    handle->filename = name;

    switch (mode) {
    case READ_MODE:
        if ((handle->file = fopen(name, READ_FILE_STRING)) == nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            handle->buffer = new char[bufferSize];
            if (handle->buffer == nullptr) {
                return (0);
            }

            setvbuf(handle->file, handle->buffer, _IOFBF, bufferSize);
        }

        for (i = 0; i < 12; i++) {
            if (getc(handle->file) == EOF) {
                return (0);
            }
        }

        if (((data1 = getc(handle->file)) == EOF) ||
            ((data2 = getc(handle->file)) == EOF)) {
            return (0);
        }

        *width = data2 * 256 + data1;

        if (((data1 = getc(handle->file)) == EOF) ||
            ((data2 = getc(handle->file)) == EOF)) {
            return (0);
        }

        for (i = 0; i < 2; i++) {
            if (getc(handle->file) == EOF) {
                return (0);
            }
        }

        *height = data2 * 256 + data1;
        handle->width = *width;
        handle->height = *height;
        handle->buffer_size = bufferSize;
        break;

    case WRITE_MODE:
        if ((handle->file = fopen(name, WRITE_FILE_STRING)) == nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            handle->buffer = new char[bufferSize];
            if (handle->buffer == nullptr) {
                return (0);
            }

            setvbuf(handle->file, handle->buffer, _IOFBF, bufferSize);
        }

        for (i = 0; i < 10; i++) { /* 00, 00, 02, then 7 00's... */
            if (i == 2) {
                putc(i, handle->file);
            } else {
                putc(0, handle->file);
            }
        }

        putc(firstLine % 256, handle->file); /* y origin set to "First_Line" */

        putc(firstLine / 256, handle->file);

        putc(*width % 256, handle->file); /* write width and height */
        putc(*width / 256, handle->file);
        putc(*height % 256, handle->file);
        putc(*height / 256, handle->file);
        putc(24, handle->file); /* 24 bits/pixel (16 million col */
        putc(32, handle->file); /* Bitmask, pertinent bit: top-d */

        handle->width = *width;
        handle->height = *height;
        handle->buffer_size = bufferSize;

        break;

    case APPEND_MODE:
        if ((handle->file = fopen(name, APPEND_FILE_STRING)) == nullptr) {
            return (0);
        }

        if (bufferSize != 0) {
            handle->buffer = new char[bufferSize];
            if (handle->buffer == nullptr) {
                return (0);
            }

            setvbuf(handle->file, handle->buffer, _IOFBF, bufferSize);
        }

        break;
    }

    return (1);
}

void
TargaFormat::writeTargaLine(
    FileHandle *handle, RGBAColor *lineData, int lineNumber)
{
    int x;

    for (x = 0; x < handle->width; x++) {
        putc((int)floor(lineData[x].Blue * 255.0), handle->file);
        putc((int)floor(lineData[x].Green * 255.0), handle->file);
        putc((int)floor(lineData[x].Red * 255.0), handle->file);
    }

    if (handle->buffer_size == 0) {
        fflush(handle->file); /* close and reopen file for */
        handle->file = freopen(handle->filename, APPEND_FILE_STRING,
            handle->file); /* integrity in case we crash*/
    }
}

int
TargaFormat::readTargaLine(
    FileHandle *handle, RGBAColor *lineData, int *lineNumber)
{
    int x;
    int data;

    for (x = 0; x < handle->width; x++) {

        /* Read the BLUE data byte.  If EOF is reached on the first character
        read, then this line hasn't been rendered yet.  Return 0.  If an EOF
        occurs somewhere within the line, this is an error - return -1. */

        if ((data = getc(handle->file)) == EOF) {
            if (x == 0) {
                return (0);
            }
            return (-1);
        }

        lineData[x].Blue = (double)data / 255.0;

        /* Read the GREEN data byte. */
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }
        lineData[x].Green = (double)data / 255.0;

        /* Read the RED data byte. */
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }
        lineData[x].Red = (double)data / 255.0;
    }

    *lineNumber = targaLineNumber++;
    return (1);
}

void
TargaFormat::closeTargaFile(FileHandle *handle)
{
    if (handle->file) {
        fclose(handle->file);
    }
    if (handle->buffer) {
        delete handle->buffer;
    }
}

int
TargaFormat::readTargaIntLine(FileHandle *handle, ImageLine *lineData)
{
    int x;
    int data;

    if (((lineData->red = new unsigned char[handle->width]) == nullptr) ||
        ((lineData->green = new unsigned char[handle->width]) == nullptr) ||
        ((lineData->blue = new unsigned char[handle->width]) == nullptr)) {
        fprintf(stderr, "Cannot allocate memory for picture: %s\n",
            handle->filename);
        PovApp::closeAll();
        exit(1);
    }

    for (x = 0; x < handle->width; x++) {
        if ((data = getc(handle->file)) == EOF) {
            if (x == 0) {
                return (0);
            }
            return (-1);
        }
        lineData->blue[x] = data;
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }
        lineData->green[x] = data;
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }
        lineData->red[x] = data;
    }
    return (1);
}

void
TargaFormat::readTargaImage(RGBAImage *image, char *name)
{
    int row;
    FileHandle handle;

    if ((handle.file = PovApp::locateFile(name, READ_FILE_STRING)) == nullptr) {
        fprintf(stderr, "Cannot open Targa file %s\n", name);
        PovApp::closeAll();
        exit(1);
    }

    TargaFormat::openTargaFile(
        &handle, name, &image->iwidth, &image->iheight, 0, READ_MODE);

    handle.width = image->iwidth;
    handle.height = image->iheight;
    image->width = (double)image->iwidth;
    image->height = (double)image->iheight;
    image->Colour_Map_Size = 0;
    image->Colour_Map = nullptr;

    image->data.rgb_lines = new ImageLine[image->iheight];
    if (image->data.rgb_lines == nullptr) {
        fprintf(stderr, "Cannot allocate memory for picture: %s\n", name);
        exit(1);
    }

    for (row = 0; row < image->iheight && TargaFormat::readTargaIntLine(&handle,
                                              &image->data.rgb_lines[row]);
        row++) {
    }
    fclose(handle.file);
}
