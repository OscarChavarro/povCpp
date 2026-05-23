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

DumpFormat::DumpFormat()
    : inputStream(nullptr), outputStream(nullptr),
      width(0), height(0), mode(0), filename(nullptr)
{
}

DumpFormat::~DumpFormat()
{
    close();
}

const char *
DumpFormat::defaultFileName()
{
    return "data.dis";
}

int
DumpFormat::open(char *name, int *w, int *h, int bufferSize, int openMode)
{
    mode = openMode;
    filename = name;
    inputStream = nullptr;
    outputStream = nullptr;

    switch (mode) {
    case READ_MODE:
        inputStream = FileLocator::locateAsStream(name);
        if (inputStream == nullptr) {
            return 0;
        }
        *w = PersistenceElement::readSignedShortLE(*inputStream);
        *h = PersistenceElement::readSignedShortLE(*inputStream);
        width = *w;
        height = *h;
        break;

    case WRITE_MODE:
        outputStream = new java::FileOutputStream(name);
        PersistenceElement::writeSignedShortLE(*outputStream, *w);
        PersistenceElement::writeSignedShortLE(*outputStream, *h);
        width = *w;
        height = *h;
        break;

    case APPEND_MODE:
        outputStream = new java::FileOutputStream(name, true);
        width = *w;
        height = *h;
        break;
    }
    return 1;
}

void
DumpFormat::writeLine(RGBAColor *lineData, int lineNumber)
{
    PersistenceElement::writeSignedShortLE(*outputStream, lineNumber);

    for (int x = 0; x < width; x++) {
        outputStream->write((int)floor(lineData[x].Red * 255.0));
    }
    for (int x = 0; x < width; x++) {
        outputStream->write((int)floor(lineData[x].Green * 255.0));
    }
    for (int x = 0; x < width; x++) {
        outputStream->write((int)floor(lineData[x].Blue * 255.0));
    }

    outputStream->flush();
}

int
DumpFormat::readLine(RGBAColor *lineData, int *lineNumber)
{
    int lo = inputStream->read();
    if (lo == -1) {
        return 0;
    }
    int hi = inputStream->read();
    if (hi == -1) {
        return -1;
    }
    *lineNumber = lo + hi * 256;

    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) {
            return -1;
        }
        lineData[i].Red = (double)data / 255.0;
    }
    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) {
            return -1;
        }
        lineData[i].Green = (double)data / 255.0;
    }
    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) {
            return -1;
        }
        lineData[i].Blue = (double)data / 255.0;
    }

    return 1;
}

int
DumpFormat::readIntLine(ImageLine *lineData, int *lineNumber)
{
    int lo = inputStream->read();
    if (lo == -1) {
        return 0;
    }
    int hi = inputStream->read();
    if (hi == -1) {
        return -1;
    }
    *lineNumber = lo + hi * 256;

    lineData->red   = new unsigned char[width];
    lineData->green = new unsigned char[width];
    lineData->blue  = new unsigned char[width];

    if (lineData->red == nullptr || lineData->green == nullptr || lineData->blue == nullptr) {
        Logger::error("Cannot allocate memory for picture: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < width; i++) {
        lineData->red[i] = lineData->green[i] = lineData->blue[i] = 0;
    }

    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) return -1;
        lineData->red[i] = (unsigned char)data;
    }
    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) return -1;
        lineData->green[i] = (unsigned char)data;
    }
    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) return -1;
        lineData->blue[i] = (unsigned char)data;
    }

    return 1;
}

void
DumpFormat::close()
{
    if (inputStream != nullptr) {
        inputStream->close();
        delete inputStream;
        inputStream = nullptr;
    }
    if (outputStream != nullptr) {
        outputStream->close();
        delete outputStream;
        outputStream = nullptr;
    }
}

void
DumpFormat::readDumpImage(RGBAImage *image, char *name)
{
    DumpFormat fmt;
    if (!fmt.open(name, &image->iwidth, &image->iheight, 0, READ_MODE)) {
        Logger::error("Cannot open dump file %s\n", name);
        exit(1);
    }

    image->width = (double)image->iwidth;
    image->height = (double)image->iheight;
    image->colourMapSize = 0;
    image->Colour_Map = nullptr;

    image->data.rgb_lines = new ImageLine[image->iheight];
    if (image->data.rgb_lines == nullptr) {
        Logger::error("Cannot allocate memory for picture: %s\n", name);
        exit(1);
    }

    ImageLine line;
    int row;
    int rc;
    while ((rc = fmt.readIntLine(&line, &row)) == 1) {
        image->data.rgb_lines[row] = line;
    }

    fmt.close();

    if (rc != 0) {
        exit(1);
    }
}
