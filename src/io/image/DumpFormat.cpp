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
 *     rr rr rr ...     - Red data for line
 *     gg gg gg ...     - Green data for line
 *     bb bb bb ...     - Blue data for line
 *
 *****************************************************************************/

#include "io/image/DumpFormat.h"
#include "common/color/RGBAColor.h"
#include "io/FileLocator.h"
#include "io/PersistenceElement.h"
#include "common/logger/Logger.h"
#include "media/ImageData.h"
#include "media/RGBAImage.h"
#include <cmath>
#include <cstdlib>

ImageFileHandle *
DumpFormat::getDumpFileInputStream()
{
    ImageFileHandle *handle = new ImageFileHandle;
    if (handle == nullptr) {
        Logger::error("Cannot allocate memory for output file handle\n");
        return nullptr;
    }

    handle->inputStream = nullptr;
    handle->outputStream = nullptr;
    handle->Default_File_Name_p = DumpFormat::defaultDumpFileName;
    handle->Open_File_p = DumpFormat::openDumpFile;
    handle->Write_Line_p = DumpFormat::writeDumpLine;
    handle->Read_Line_p = DumpFormat::readDumpLine;
    handle->Read_Image_p = DumpFormat::readDumpImage;
    handle->Close_File_p = DumpFormat::closeDumpFile;
    return handle;
}

char *
DumpFormat::defaultDumpFileName()
{
    return (char *)("data.dis");
}

int
DumpFormat::openDumpFile(ImageFileHandle *handle, char *name, int *width,
    int *height, int bufferSize, int mode)
{
    handle->mode = mode;
    handle->filename = name;
    handle->buffer_size = bufferSize;
    handle->inputStream = nullptr;
    handle->outputStream = nullptr;

    switch (mode) {
    case ImageFileHandle::READ_MODE:
        handle->inputStream = FileLocator::locateAsStream(name);
        if (handle->inputStream == nullptr) {
            return 0;
        }
        {
            java::FileInputStream &is = *handle->inputStream;
            *width = PersistenceElement::readSignedShortLE(is);
            *height = PersistenceElement::readSignedShortLE(is);
            handle->width = *width;
            handle->height = *height;
        }
        break;

    case ImageFileHandle::WRITE_MODE:
        handle->outputStream = new java::FileOutputStream(name);
        {
            java::FileOutputStream &os = *handle->outputStream;
            PersistenceElement::writeSignedShortLE(os, *width);
            PersistenceElement::writeSignedShortLE(os, *height);
        }
        handle->width = *width;
        handle->height = *height;
        break;

    case ImageFileHandle::APPEND_MODE:
        handle->outputStream = new java::FileOutputStream(name, true);
        handle->width = *width;
        handle->height = *height;
        break;
    }
    return 1;
}

void
DumpFormat::writeDumpLine(
    ImageFileHandle *handle, RGBAColor *lineData, int lineNumber)
{
    java::FileOutputStream &os = *handle->outputStream;

    PersistenceElement::writeSignedShortLE(os, lineNumber);

    for (int x = 0; x < handle->width; x++) {
        os.write((int)floor(lineData[x].Red * 255.0));
    }
    for (int x = 0; x < handle->width; x++) {
        os.write((int)floor(lineData[x].Green * 255.0));
    }
    for (int x = 0; x < handle->width; x++) {
        os.write((int)floor(lineData[x].Blue * 255.0));
    }

    os.flush();
}

int
DumpFormat::readDumpLine(
    ImageFileHandle *handle, RGBAColor *lineData, int *lineNumber)
{
    java::FileInputStream &is = *handle->inputStream;

    int lo = is.read();
    if (lo == -1) {
        return 0;
    }
    int hi = is.read();
    if (hi == -1) {
        return -1;
    }
    *lineNumber = lo + hi * 256;

    for (int i = 0; i < handle->width; i++) {
        int data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData[i].Red = (double)data / 255.0;
    }
    for (int i = 0; i < handle->width; i++) {
        int data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData[i].Green = (double)data / 255.0;
    }
    for (int i = 0; i < handle->width; i++) {
        int data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData[i].Blue = (double)data / 255.0;
    }

    return 1;
}

int
DumpFormat::readDumpIntLine(
    ImageFileHandle *handle, ImageLine *lineData, int *lineNumber)
{
    java::FileInputStream &is = *handle->inputStream;

    int lo = is.read();
    if (lo == -1) {
        return 0;
    }
    int hi = is.read();
    if (hi == -1) {
        return -1;
    }
    *lineNumber = lo + hi * 256;

    lineData->red = new unsigned char[handle->width];
    lineData->green = new unsigned char[handle->width];
    lineData->blue = new unsigned char[handle->width];

    if (lineData->red == nullptr || lineData->green == nullptr || lineData->blue == nullptr) {
        Logger::error("Cannot allocate memory for picture: %s\n", handle->filename);
        exit(1);
    }

    for (int i = 0; i < handle->width; i++) {
        lineData->red[i] = 0;
        lineData->green[i] = 0;
        lineData->blue[i] = 0;
    }

    for (int i = 0; i < handle->width; i++) {
        int data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData->red[i] = (unsigned char)data;
    }
    for (int i = 0; i < handle->width; i++) {
        int data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData->green[i] = (unsigned char)data;
    }
    for (int i = 0; i < handle->width; i++) {
        int data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData->blue[i] = (unsigned char)data;
    }

    return 1;
}

void
DumpFormat::closeDumpFile(ImageFileHandle *handle)
{
    if (handle->inputStream != nullptr) {
        handle->inputStream->close();
        delete handle->inputStream;
        handle->inputStream = nullptr;
    }
    if (handle->outputStream != nullptr) {
        handle->outputStream->close();
        delete handle->outputStream;
        handle->outputStream = nullptr;
    }
}

void
DumpFormat::readDumpImage(RGBAImage *image, char *name)
{
    ImageFileHandle handle;
    handle.inputStream = nullptr;
    handle.outputStream = nullptr;
    handle.filename = name;

    if (!DumpFormat::openDumpFile(&handle, name, &image->iwidth, &image->iheight, 0, ImageFileHandle::READ_MODE)) {
        Logger::error("Cannot open dump file %s\n", name);
        exit(1);
    }

    image->width = (double)image->iwidth;
    image->height = (double)image->iheight;
    image->Colour_Map_Size = 0;
    image->Colour_Map = nullptr;

    image->data.rgb_lines = new ImageLine[image->iheight];
    if (image->data.rgb_lines == nullptr) {
        Logger::error("Cannot allocate memory for picture: %s\n", name);
        exit(1);
    }

    ImageLine line;
    int row;
    int rc;
    while ((rc = DumpFormat::readDumpIntLine(&handle, &line, &row)) == 1) {
        image->data.rgb_lines[row] = line;
    }

    DumpFormat::closeDumpFile(&handle);

    if (rc == 0) {
        return;
    }
    exit(1);
}
