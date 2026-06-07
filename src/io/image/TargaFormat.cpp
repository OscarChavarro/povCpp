/****************************************************************************
 *                     targa.c
 *
 *  This module contains the code to read and write the Targa output file
 *  format.
 *
 *****************************************************************************/

#include "io/image/TargaFormat.h"
#include "io/binaryIo/FileLocator.h"
#include "common/logger/Logger.h"
#include "common/color/RGBAColor.h"
#include "java/io/FileOutputStream.h"
#include "media/ImageData.h"
#include "media/RGBAImage.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace {

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

}

TargaFormat::TargaFormat()
    : inputStream(nullptr), outputStream(nullptr),
      width(0), height(0), mode(0), filename(nullptr)
{
}

TargaFormat::~TargaFormat()
{
    close();
}

const char *
TargaFormat::defaultFileName()
{
    return "data.tga";
}

int
TargaFormat::open(char *name, int *w, int *h, int bufferSize, int openMode, int firstLine)
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
        for (int i = 0; i < 12; i++) {
            if (inputStream->read() == -1) {
                return 0;
            }
        }
        {
            int data1 = inputStream->read();
            int data2 = inputStream->read();
            if (data1 == -1 || data2 == -1) {
                return 0;
            }
            *w = data2 * 256 + data1;

            data1 = inputStream->read();
            data2 = inputStream->read();
            if (data1 == -1 || data2 == -1) {
                return 0;
            }
            if (inputStream->read() == -1 || inputStream->read() == -1) {
                return 0;
            }
            *h = data2 * 256 + data1;
        }
        width = *w;
        height = *h;
        break;

    case WRITE_MODE:
        outputStream = new java::FileOutputStream(name);
        if (!outputStream) {
            return 0;
        }
        for (int i = 0; i < 10; i++) {
            outputStream->write(i == 2 ? 2 : 0);
        }
        outputStream->write(firstLine % 256);
        outputStream->write(firstLine / 256);
        outputStream->write(*w % 256);
        outputStream->write(*w / 256);
        outputStream->write(*h % 256);
        outputStream->write(*h / 256);
        outputStream->write(24);
        outputStream->write(32);
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
TargaFormat::writeLine(RGBAColor *lineData, int lineNumber)
{
    for (int x = 0; x < width; x++) {
        outputStream->write((int)floor(lineData[x].Blue * 255.0));
        outputStream->write((int)floor(lineData[x].Green * 255.0));
        outputStream->write((int)floor(lineData[x].Red * 255.0));
    }

    outputStream->flush();
}

int
TargaFormat::readLine(RGBAColor *lineData, int *lineNumber)
{
    for (int x = 0; x < width; x++) {
        int data = inputStream->read();
        if (data == -1) {
            return (x == 0) ? 0 : -1;
        }
        lineData[x].Blue = (double)data / 255.0;

        data = inputStream->read();
        if (data == -1) return -1;
        lineData[x].Green = (double)data / 255.0;

        data = inputStream->read();
        if (data == -1) return -1;
        lineData[x].Red = (double)data / 255.0;
    }

    return 1;
}

int
TargaFormat::readIntLine(ImageLine *lineData)
{
    lineData->red   = new unsigned char[width];
    lineData->green = new unsigned char[width];
    lineData->blue  = new unsigned char[width];

    if (lineData->red == nullptr || lineData->green == nullptr || lineData->blue == nullptr) {
        Logger::error("Cannot allocate memory for picture: %s\n", filename);
        exit(1);
    }

    for (int x = 0; x < width; x++) {
        int data = inputStream->read();
        if (data == -1) {
            return (x == 0) ? 0 : -1;
        }
        lineData->blue[x] = (unsigned char)data;

        data = inputStream->read();
        if (data == -1) return -1;
        lineData->green[x] = (unsigned char)data;

        data = inputStream->read();
        if (data == -1) return -1;
        lineData->red[x] = (unsigned char)data;
    }
    return 1;
}

void
TargaFormat::close()
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
TargaFormat::readTargaImage(RGBAImage *image, char *name)
{
    TargaFormat fmt;
    if (!fmt.open(name, &image->iwidth, &image->iheight, 0, READ_MODE, 0)) {
        Logger::error("Cannot open Targa file %s\n", name);
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

    for (int row = 0; row < image->iheight &&
         fmt.readIntLine(&image->data.rgb_lines[row]);
         row++) {
    }

    fmt.close();
}
