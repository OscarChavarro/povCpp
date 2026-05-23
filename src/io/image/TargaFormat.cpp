/****************************************************************************
 *                     targa.c
 *
 *  This module contains the code to read and write the Targa output file
 *  format.
 *
 *****************************************************************************/

#include "io/image/TargaFormat.h"
#include "io/FileLocator.h"
#include "io/PersistenceElement.h"
#include "environment/material/RendererConfiguration.h"
#include "common/logger/Logger.h"
#include "common/color/RGBAColor.h"
#include "media/ImageData.h"
#include "media/RGBAImage.h"
#include <cmath>
#include <cstdlib>

ImageFileHandle *
TargaFormat::getTargaFileInputStream()
{
    ImageFileHandle *handle = new ImageFileHandle;
    if (handle == nullptr) {
        Logger::error("Cannot allocate memory for output file handle\n");
        return nullptr;
    }

    handle->inputStream = nullptr;
    handle->outputStream = nullptr;
    handle->Default_File_Name_p = TargaFormat::defaultTargaFileName;
    handle->Open_File_p = TargaFormat::openTargaFile;
    handle->Write_Line_p = TargaFormat::writeTargaLine;
    handle->Read_Line_p = TargaFormat::readTargaLine;
    handle->Read_Image_p = TargaFormat::readTargaImage;
    handle->Close_File_p = TargaFormat::closeTargaFile;
    handle->buffer_size = 0;
    return handle;
}

static char defaultTargaFilename[] = "data.tga";

char *
TargaFormat::defaultTargaFileName()
{
    return defaultTargaFilename;
}

int
TargaFormat::openTargaFile(ImageFileHandle *handle, char *name, int *width,
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
            for (int i = 0; i < 12; i++) {
                if (is.read() == -1) {
                    return 0;
                }
            }
            int data1 = is.read();
            int data2 = is.read();
            if (data1 == -1 || data2 == -1) {
                return 0;
            }
            *width = data2 * 256 + data1;

            data1 = is.read();
            data2 = is.read();
            if (data1 == -1 || data2 == -1) {
                return 0;
            }
            if (is.read() == -1 || is.read() == -1) {
                return 0;
            }
            *height = data2 * 256 + data1;
            handle->width = *width;
            handle->height = *height;
        }
        break;

    case ImageFileHandle::WRITE_MODE:
        handle->outputStream = new java::FileOutputStream(name);
        if (!handle->outputStream) {
            return 0;
        }
        {
            java::FileOutputStream &os = *handle->outputStream;
            for (int i = 0; i < 10; i++) {
                os.write(i == 2 ? 2 : 0);
            }
            os.write(globalRenderingConfiguration.firstLine % 256);
            os.write(globalRenderingConfiguration.firstLine / 256);
            os.write(*width % 256);
            os.write(*width / 256);
            os.write(*height % 256);
            os.write(*height / 256);
            os.write(24);
            os.write(32);
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
TargaFormat::writeTargaLine(
    ImageFileHandle *handle, RGBAColor *lineData, int lineNumber)
{
    java::FileOutputStream &os = *handle->outputStream;

    for (int x = 0; x < handle->width; x++) {
        os.write((int)floor(lineData[x].Blue * 255.0));
        os.write((int)floor(lineData[x].Green * 255.0));
        os.write((int)floor(lineData[x].Red * 255.0));
    }

    os.flush();
}

int
TargaFormat::readTargaLine(
    ImageFileHandle *handle, RGBAColor *lineData, int *lineNumber)
{
    java::FileInputStream &is = *handle->inputStream;

    for (int x = 0; x < handle->width; x++) {
        int data = is.read();
        if (data == -1) {
            if (x == 0) {
                return 0;
            }
            return -1;
        }
        lineData[x].Blue = (double)data / 255.0;

        data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData[x].Green = (double)data / 255.0;

        data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData[x].Red = (double)data / 255.0;
    }

    return 1;
}

void
TargaFormat::closeTargaFile(ImageFileHandle *handle)
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

int
TargaFormat::readTargaIntLine(ImageFileHandle *handle, ImageLine *lineData)
{
    java::FileInputStream &is = *handle->inputStream;

    lineData->red = new unsigned char[handle->width];
    lineData->green = new unsigned char[handle->width];
    lineData->blue = new unsigned char[handle->width];

    if (lineData->red == nullptr || lineData->green == nullptr || lineData->blue == nullptr) {
        Logger::error("Cannot allocate memory for picture: %s\n", handle->filename);
        exit(1);
    }

    for (int x = 0; x < handle->width; x++) {
        int data = is.read();
        if (data == -1) {
            if (x == 0) {
                return 0;
            }
            return -1;
        }
        lineData->blue[x] = (unsigned char)data;

        data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData->green[x] = (unsigned char)data;

        data = is.read();
        if (data == -1) {
            return -1;
        }
        lineData->red[x] = (unsigned char)data;
    }
    return 1;
}

void
TargaFormat::readTargaImage(RGBAImage *image, char *name)
{
    ImageFileHandle handle;
    handle.inputStream = nullptr;
    handle.outputStream = nullptr;
    handle.filename = name;

    if (!TargaFormat::openTargaFile(
            &handle, name, &image->iwidth, &image->iheight, 0, ImageFileHandle::READ_MODE)) {
        Logger::error("Cannot open Targa file %s\n", name);
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

    for (int row = 0; row < image->iheight &&
         TargaFormat::readTargaIntLine(&handle, &image->data.rgb_lines[row]);
         row++) {
    }

    TargaFormat::closeTargaFile(&handle);
}
