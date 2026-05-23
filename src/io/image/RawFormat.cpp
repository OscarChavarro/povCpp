/****************************************************************************
 *                     raw.c
 *
 *  This module contains the code to read and write the RAW file format.
 *
 *****************************************************************************/

#include "io/image/RawFormat.h"
#include "common/logger/Logger.h"
#include "common/color/RGBAColor.h"
#include <cmath>
#include <cstring>

class RAW_FILE_HANDLE {
  public:
    ImageFileHandle root;
    java::FileInputStream  *redIn,   *greenIn,   *blueIn;
    java::FileOutputStream *redOut,  *greenOut,  *blueOut;
    int lineNumber;
};

ImageFileHandle *
RawFormat::getRawFileInputStream()
{
    RAW_FILE_HANDLE *handle = new RAW_FILE_HANDLE;
    if (handle == nullptr) {
        Logger::error("Cannot allocate memory for output file handle\n");
        return nullptr;
    }

    handle->redIn = handle->greenIn = handle->blueIn = nullptr;
    handle->redOut = handle->greenOut = handle->blueOut = nullptr;
    handle->root.inputStream = nullptr;
    handle->root.outputStream = nullptr;
    handle->root.Default_File_Name_p = RawFormat::defaultRawFileName;
    handle->root.Open_File_p = RawFormat::openRawFile;
    handle->root.Write_Line_p = RawFormat::writeRawLine;
    handle->root.Read_Line_p = RawFormat::readRawLine;
    handle->root.Close_File_p = RawFormat::closeRawFile;
    return (ImageFileHandle *)handle;
}

static char defaultNameData[] = "data";

char *
RawFormat::defaultRawFileName()
{
    return defaultNameData;
}

static java::FileInputStream *openRawInputStream(const char *base, const char *ext)
{
    char fileName[256];
    std::strcpy(fileName, base);
    std::strcat(fileName, ext);
    java::FileInputStream *s = new java::FileInputStream(fileName);
    if (!s->isOpen()) {
        delete s;
        return nullptr;
    }
    return s;
}

static java::FileOutputStream *openRawOutputStream(const char *base, const char *ext, bool append)
{
    char fileName[256];
    std::strcpy(fileName, base);
    std::strcat(fileName, ext);
    return new java::FileOutputStream(fileName, append);
}

int
RawFormat::openRawFile(ImageFileHandle *handle, char *name, int *width,
    int *height, int bufferSize, int mode)
{
    RAW_FILE_HANDLE *raw = (RAW_FILE_HANDLE *)handle;

    handle->mode = mode;
    handle->filename = name;
    handle->buffer_size = bufferSize;
    raw->lineNumber = 0;
    raw->redIn = raw->greenIn = raw->blueIn = nullptr;
    raw->redOut = raw->greenOut = raw->blueOut = nullptr;

    switch (mode) {
    case ImageFileHandle::READ_MODE:
        raw->redIn   = openRawInputStream(name, RED_RAW_FILE_EXTENSION);
        raw->greenIn = openRawInputStream(name, GREEN_RAW_FILE_EXTENSION);
        raw->blueIn  = openRawInputStream(name, BLUE_RAW_FILE_EXTENSION);
        if (!raw->redIn || !raw->greenIn || !raw->blueIn) {
            return 0;
        }
        handle->width  = *width;
        handle->height = *height;
        break;

    case ImageFileHandle::WRITE_MODE:
        raw->redOut   = openRawOutputStream(name, RED_RAW_FILE_EXTENSION,   false);
        raw->greenOut = openRawOutputStream(name, GREEN_RAW_FILE_EXTENSION, false);
        raw->blueOut  = openRawOutputStream(name, BLUE_RAW_FILE_EXTENSION,  false);
        handle->width  = *width;
        handle->height = *height;
        break;

    case ImageFileHandle::APPEND_MODE:
        raw->redOut   = openRawOutputStream(name, RED_RAW_FILE_EXTENSION,   true);
        raw->greenOut = openRawOutputStream(name, GREEN_RAW_FILE_EXTENSION, true);
        raw->blueOut  = openRawOutputStream(name, BLUE_RAW_FILE_EXTENSION,  true);
        handle->width  = *width;
        handle->height = *height;
        break;
    }
    return 1;
}

void
RawFormat::writeRawLine(ImageFileHandle *handle, RGBAColor *lineData, int lineNumber)
{
    RAW_FILE_HANDLE *raw = (RAW_FILE_HANDLE *)handle;

    for (int x = 0; x < handle->width; x++) {
        raw->redOut->write((int)floor(lineData[x].Red * 255.0));
    }
    for (int x = 0; x < handle->width; x++) {
        raw->greenOut->write((int)floor(lineData[x].Green * 255.0));
    }
    for (int x = 0; x < handle->width; x++) {
        raw->blueOut->write((int)floor(lineData[x].Blue * 255.0));
    }

    raw->redOut->flush();
    raw->greenOut->flush();
    raw->blueOut->flush();
}

int
RawFormat::readRawLine(ImageFileHandle *handle, RGBAColor *lineData, int *lineNumber)
{
    RAW_FILE_HANDLE *raw = (RAW_FILE_HANDLE *)handle;

    for (int i = 0; i < handle->width; i++) {
        int data = raw->redIn->read();
        if (data == -1) {
            if (i == 0) {
                return 0;
            }
            return -1;
        }
        lineData[i].Red = (double)data / 255.0;
    }
    for (int i = 0; i < handle->width; i++) {
        int data = raw->greenIn->read();
        if (data == -1) {
            return -1;
        }
        lineData[i].Green = (double)data / 255.0;
    }
    for (int i = 0; i < handle->width; i++) {
        int data = raw->blueIn->read();
        if (data == -1) {
            return -1;
        }
        lineData[i].Blue = (double)data / 255.0;
    }

    *lineNumber = raw->lineNumber++;
    return 1;
}

void
RawFormat::closeRawFile(ImageFileHandle *handle)
{
    RAW_FILE_HANDLE *raw = (RAW_FILE_HANDLE *)handle;

    if (raw->redIn)   { raw->redIn->close();   delete raw->redIn;   raw->redIn = nullptr; }
    if (raw->greenIn) { raw->greenIn->close(); delete raw->greenIn; raw->greenIn = nullptr; }
    if (raw->blueIn)  { raw->blueIn->close();  delete raw->blueIn;  raw->blueIn = nullptr; }

    if (raw->redOut)   { raw->redOut->close();   delete raw->redOut;   raw->redOut = nullptr; }
    if (raw->greenOut) { raw->greenOut->close(); delete raw->greenOut; raw->greenOut = nullptr; }
    if (raw->blueOut)  { raw->blueOut->close();  delete raw->blueOut;  raw->blueOut = nullptr; }
}
