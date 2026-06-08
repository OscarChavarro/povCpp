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

#include "io/image/RawDumpFormat.h"
#include "common/color/RGBAColor.h"
#include "io/binaryIo/FileLocator.h"
#include "common/logger/Logger.h"
#include "java/io/FileOutputStream.h"
#include "vsdk/toolkit/io/PersistenceElement.h"
#include "media/ImageData.h"
#include "media/RGBAImage.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

class AppendableFileOutputStream : public java::OutputStream {
  public:
    explicit AppendableFileOutputStream(const char *fileName)
        : stream(nullptr)
    {
        if (fileName != nullptr && fileName[0] != '\0') {
            stream = std::fopen(fileName, "ab");
        }
    }

    ~AppendableFileOutputStream() override
    {
        close();
    }

    void write(int value) override
    {
        if (stream != nullptr) {
            std::fputc(static_cast<unsigned char>(value & 0xFF), stream);
        }
    }

    void write(const unsigned char *buffer, int offset, int length) override
    {
        if (stream != nullptr && buffer != nullptr && offset >= 0 && length > 0) {
            std::fwrite(buffer + offset, 1, static_cast<size_t>(length), stream);
        }
    }

    void flush() override
    {
        if (stream != nullptr) {
            std::fflush(stream);
        }
    }

    void close() override
    {
        if (stream != nullptr) {
            std::fclose(stream);
            stream = nullptr;
        }
    }

  private:
    FILE *stream;
};

RawDumpFormat::RawDumpFormat()
    : inputStream(nullptr), outputStream(nullptr),
      width(0), height(0), mode(0), filename(nullptr)
{
}

RawDumpFormat::~RawDumpFormat()
{
    close();
}

const char *
RawDumpFormat::defaultFileName()
{
    return "data.dis";
}

int
RawDumpFormat::open(char *name, int *w, int *h, int bufferSize, int openMode, int /*firstLine*/)
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
        outputStream = new AppendableFileOutputStream(name);
        width = *w;
        height = *h;
        break;
    }
    return 1;
}

void
RawDumpFormat::writeLine(RGBAColor *lineData, int lineNumber)
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
RawDumpFormat::readLine(RGBAColor *lineData, int *lineNumber)
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
RawDumpFormat::readIntLine(ImageLine *lineData, int *lineNumber)
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

    lineData->r   = new unsigned char[width];
    lineData->g = new unsigned char[width];
    lineData->b  = new unsigned char[width];

    if (lineData->r == nullptr || lineData->g == nullptr || lineData->b == nullptr) {
        Logger::error("Cannot allocate memory for picture: %s\n", filename);
        exit(1);
    }

    for (int i = 0; i < width; i++) {
        lineData->r[i] = lineData->g[i] = lineData->b[i] = 0;
    }

    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) return -1;
        lineData->r[i] = (unsigned char)data;
    }
    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) return -1;
        lineData->g[i] = (unsigned char)data;
    }
    for (int i = 0; i < width; i++) {
        int data = inputStream->read();
        if (data == -1) return -1;
        lineData->b[i] = (unsigned char)data;
    }

    return 1;
}

void
RawDumpFormat::close()
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
RawDumpFormat::readDumpImage(RGBAImage *image, char *name)
{
    RawDumpFormat fmt;
    if (!fmt.open(name, &image->iwidth, &image->iheight, 0, READ_MODE, 0)) {
        Logger::error("Cannot open dump file %s\n", name);
        exit(1);
    }

    image->width = (double)image->iwidth;
    image->height = (double)image->iheight;
    image->colourMapSize = 0;
    image->colorMap = nullptr;

    image->data.lines = new ImageLine[image->iheight];
    if (image->data.lines == nullptr) {
        Logger::error("Cannot allocate memory for picture: %s\n", name);
        exit(1);
    }

    ImageLine line;
    int row;
    int rc;
    while ((rc = fmt.readIntLine(&line, &row)) == 1) {
        image->data.lines[row] = line;
    }

    fmt.close();

    if (rc != 0) {
        exit(1);
    }
}
