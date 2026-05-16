/****************************************************************************
 *                         dump.c
 *
 *  This module contains the code to read and write the dump file format.
 *  The format is as follows:
 *
 *  (header:)
 *     wwww hhhh         - Width, Height (16 bits, LSB first)
 *
 *  (each scanline:)
 *     llll                - Line number (16 bits, LSB first)
 *     rr rr rr ...     - Red data for line (8 bits per pixel,
 *                              left to right, 0-255 (255=bright, 0=dark))
 *     gg gg gg ...     - Green data for line (8 bits per pixel,
 *                              left to right, 0-255 (255=bright, 0=dark))
 *     bb bb bb ...     - Blue data for line (8 bits per pixel,
 *                              left to right, 0-255 (255=bright, 0=dark))
 *
 *
 *
 *  from Persistence of Vision Raytracer
 *  Copyright 1992 Persistence of Vision Team
 *---------------------------------------------------------------------------
 *  Copying, distribution and legal info is in the file povlegal.doc which
 *  should be distributed with this file. If povlegal.doc is not available
 *  or for more info please contact:
 *
 *         Drew Wells [POV-Team Leader]
 *         CIS: 73767,1244  Internet: 73767.1244@compuserve.com
 *         Phone: (213) 254-4041
 *
 * This program is based on the popular DKB raytracer version 2.12.
 * DKBTrace was originally written by David K. Buck.
 * DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
 *
 *****************************************************************************/

#include "io/Dump.h"
#include "common/PovProto.h"
#include "geom/Geometry.h"

FileHandle *
Get_Dump_File_Handle()
{
    FileHandle *handle;

    handle = new FileHandle;
    if (handle == nullptr) {
        fprintf(stderr, "Cannot allocate memory for output file handle\n");
        return (nullptr);
    }

    handle->Default_File_Name_p = Default_Dump_File_Name;
    handle->Open_File_p = Open_Dump_File;
    handle->Write_Line_p = Write_Dump_Line;
    handle->Read_Line_p = Read_Dump_Line;
    handle->Read_Image_p = Read_Dump_Image;
    handle->Close_File_p = Close_Dump_File;
    return (handle);
}

char *
Default_Dump_File_Name()
{
    return (char *)("data.dis");
}

int
Open_Dump_File(FileHandle *handle, char *name, int *width, int *height,
    int bufferSize, int mode)
{
    int data1;
    int data2;

    handle->mode = mode;
    handle->filename = name;

    switch (mode) {
    case READ_MODE:
        if ((handle->file = fopen(name, READ_FILE_STRING)) == nullptr) {
            return 0;
        }

        if (bufferSize != 0) {
            handle->buffer = new char[bufferSize];
            if (handle->buffer == nullptr) {
                return 0;
            }

            setvbuf(handle->file, handle->buffer, _IOFBF, bufferSize);
        }

        if (((data1 = getc(handle->file)) == EOF) ||
            ((data2 = getc(handle->file)) == EOF)) {
            return 0;
        }

        *width = data2 * 256 + data1;

        if (((data1 = getc(handle->file)) == EOF) ||
            ((data2 = getc(handle->file)) == EOF)) {
            return (0);
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

        putc(*width % 256, handle->file); /* write to either type of file */
        putc(*width / 256, handle->file);
        putc(*height % 256, handle->file);
        putc(*height / 256, handle->file);

        handle->width = *width;
        handle->height = *height;
        handle->buffer_size = bufferSize;

        break;

    case APPEND_MODE:
        if ((handle->file = fopen(name, APPEND_FILE_STRING)) == nullptr) {
            return 0;
        }

        if (bufferSize != 0) {
            handle->buffer = new char[bufferSize];
            if (handle->buffer == nullptr) {
                return 0;
            }

            setvbuf(handle->file, handle->buffer, _IOFBF, bufferSize);
        }

        handle->buffer_size = bufferSize;
        break;
    }
    return 1;
}

void
Write_Dump_Line(FileHandle *handle, RGBAColor *lineData, int lineNumber)
{
    register int x;

    putc(lineNumber % 256, handle->file);
    putc(lineNumber / 256, handle->file);

    for (x = 0; x < handle->width; x++) {
        putc((int)floor(lineData[x].Red * 255.0), handle->file);
    }

    for (x = 0; x < handle->width; x++) {
        putc((int)floor(lineData[x].Green * 255.0), handle->file);
    }

    for (x = 0; x < handle->width; x++) {
        putc((int)floor(lineData[x].Blue * 255.0), handle->file);
    }

    if (handle->buffer_size == 0) {
        fflush(handle->file); /* close and reopen file for */
        handle->file = freopen(handle->filename, APPEND_FILE_STRING,
            handle->file); /* integrity in case we crash*/
    }
}

int
Read_Dump_Line(FileHandle *handle, RGBAColor *lineData, int *lineNumber)
{
    int data;
    int i;
    int c;

    if ((c = getc(handle->file)) == EOF) {
        return (0);
    }

    *lineNumber = c;

    if ((c = getc(handle->file)) == EOF) {
        return (-1);
    }

    *lineNumber += c * 256;

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }

        lineData[i].Red = (DBL)data / 255.0;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }

        lineData[i].Green = (DBL)data / 255.0;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }

        lineData[i].Blue = (DBL)data / 255.0;
    }

    return (1);
}

int
Read_Dump_Int_Line(FileHandle *handle, ImageLine *lineData, int *lineNumber)
{
    int data;
    int i;
    int c;

    if ((c = getc(handle->file)) == EOF) {
        return (0);
    }

    *lineNumber = c;

    if ((c = getc(handle->file)) == EOF) {
        return (-1);
    }

    *lineNumber += c * 256;

    if (((lineData->red = new unsigned char[handle->width]) == nullptr) ||
        ((lineData->green = new unsigned char[handle->width]) == nullptr) ||
        ((lineData->blue = new unsigned char[handle->width]) == nullptr)) {
        fprintf(stderr, "Cannot allocate memory for picture: %s\n",
            handle->filename);
        close_all();
        exit(1);
    }

    for (i = 0; i < handle->width; i++) {
        lineData->red[i] = 0;
        lineData->green[i] = 0;
        lineData->blue[i] = 0;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }

        lineData->red[i] = (unsigned char)data;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }

        lineData->green[i] = (unsigned char)data;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }

        lineData->blue[i] = (unsigned char)data;
    }

    return (1);
}

void
Close_Dump_File(FileHandle *handle)
{
    if (handle->file) {
        fclose(handle->file);
    }
    if (handle->buffer_size != 0) {
        delete handle->buffer;
    }
}

void
Read_Dump_Image(RGBAImage *image, char *name)
{
    int rc;
    int row;
    int data1;
    int data2;
    ImageLine line;
    FileHandle handle;

    if ((handle.file = Locate_File(name, READ_FILE_STRING)) == nullptr) {
        fprintf(stderr, "Cannot open dump file %s\n", name);
        close_all();
        exit(1);
    }

    if (((data1 = getc(handle.file)) == EOF) ||
        ((data2 = getc(handle.file)) == EOF)) {

        fprintf(stderr, "Cannot open dump file %s\n", name);
        close_all();
        exit(1);
    }

    image->iwidth = data2 * 256 + data1;
    handle.width = image->iwidth;

    if (((data1 = getc(handle.file)) == EOF) ||
        ((data2 = getc(handle.file)) == EOF)) {

        fprintf(stderr, "Cannot open dump file %s\n", name);
        close_all();
        exit(1);
    }

    image->iheight = data2 * 256 + data1;
    handle.height = image->iheight;

    image->width = (DBL)image->iwidth;
    image->height = (DBL)image->iheight;

    image->Colour_Map_Size = 0;
    image->Colour_Map = nullptr;

    image->data.rgb_lines = new ImageLine[image->iheight];
    if (image->data.rgb_lines == nullptr) {
        fprintf(stderr, "Cannot allocate memory for picture: %s\n", name);
        exit(1);
    }

    while ((rc = Read_Dump_Int_Line(&handle, &line, &row)) == 1) {
        image->data.rgb_lines[row] = line;
    }

    fclose(handle.file);

    if (rc == 0) {
        return;
    }
    close_all();
    exit(1);
}
