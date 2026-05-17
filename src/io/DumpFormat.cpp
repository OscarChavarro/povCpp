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
 *****************************************************************************/

#include "io/DumpFormat.h"
#include "app/PovApp.h"
#include "geom/GeometryOperations.h"

FileHandle *
DumpFormat::getDumpFileHandle()
{
    FileHandle *handle;

    handle = new FileHandle;
    if (handle == nullptr) {
        fprintf(stderr, "Cannot allocate memory for output file handle\n");
        return (nullptr);
    }

    handle->Default_File_Name_p = DumpFormat::defaultDumpFileName;
    handle->Open_File_p = DumpFormat::openDumpFile;
    handle->Write_Line_p = DumpFormat::writeDumpLine;
    handle->Read_Line_p = DumpFormat::readDumpLine;
    handle->Read_Image_p = DumpFormat::readDumpImage;
    handle->Close_File_p = DumpFormat::closeDumpFile;
    return (handle);
}

char *
DumpFormat::defaultDumpFileName()
{
    return (char *)("data.dis");
}

int
DumpFormat::openDumpFile(FileHandle *handle, char *name, int *width, int *height,
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
DumpFormat::writeDumpLine(FileHandle *handle, RGBAColor *lineData, int lineNumber)
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
DumpFormat::readDumpLine(FileHandle *handle, RGBAColor *lineData, int *lineNumber)
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

        lineData[i].Red = (double)data / 255.0;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }

        lineData[i].Green = (double)data / 255.0;
    }

    for (i = 0; i < handle->width; i++) {
        if ((data = getc(handle->file)) == EOF) {
            return (-1);
        }

        lineData[i].Blue = (double)data / 255.0;
    }

    return (1);
}

int
DumpFormat::readDumpIntLine(FileHandle *handle, ImageLine *lineData, int *lineNumber)
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
        PovApp::closeAll();
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
DumpFormat::closeDumpFile(FileHandle *handle)
{
    if (handle->file) {
        fclose(handle->file);
    }
    if (handle->buffer_size != 0) {
        delete handle->buffer;
    }
}

void
DumpFormat::readDumpImage(RGBAImage *image, char *name)
{
    int rc;
    int row;
    int data1;
    int data2;
    ImageLine line;
    FileHandle handle;

    if ((handle.file = PovApp::locateFile(name, READ_FILE_STRING)) == nullptr) {
        fprintf(stderr, "Cannot open dump file %s\n", name);
        PovApp::closeAll();
        exit(1);
    }

    if (((data1 = getc(handle.file)) == EOF) ||
        ((data2 = getc(handle.file)) == EOF)) {

        fprintf(stderr, "Cannot open dump file %s\n", name);
        PovApp::closeAll();
        exit(1);
    }

    image->iwidth = data2 * 256 + data1;
    handle.width = image->iwidth;

    if (((data1 = getc(handle.file)) == EOF) ||
        ((data2 = getc(handle.file)) == EOF)) {

        fprintf(stderr, "Cannot open dump file %s\n", name);
        PovApp::closeAll();
        exit(1);
    }

    image->iheight = data2 * 256 + data1;
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

    while ((rc = DumpFormat::readDumpIntLine(&handle, &line, &row)) == 1) {
        image->data.rgb_lines[row] = line;
    }

    fclose(handle.file);

    if (rc == 0) {
        return;
    }
    PovApp::closeAll();
    exit(1);
}
