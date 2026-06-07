/****************************************************************************
 *                     raw.c
 *
 *  This module contains the code to read and write the RAW file format.
 *
 *****************************************************************************/

#include "io/image/RawFormat.h"
#include "common/logger/Logger.h"
#include "common/color/RGBAColor.h"
#include "java/io/FileOutputStream.h"
#include <cmath>
#include <cstring>
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

RawFormat::RawFormat()
    : redIn(nullptr), greenIn(nullptr), blueIn(nullptr),
      redOut(nullptr), greenOut(nullptr), blueOut(nullptr),
      width(0), height(0), mode(0), filename(nullptr), lineCounter(0)
{
}

RawFormat::~RawFormat()
{
    close();
}

const char *
RawFormat::defaultFileName()
{
    return "data";
}

java::FileInputStream *
RawFormat::openRawInputStream(const char *base, const char *ext)
{
    char fileName[256];
    std::strcpy(fileName, base);
    std::strcat(fileName, ext);
    java::FileInputStream *s = new java::FileInputStream(fileName);
    java::File probe(fileName);
    if (!probe.canRead()) {
        delete s;
        return nullptr;
    }
    return s;
}

java::OutputStream *
RawFormat::openRawOutputStream(const char *base, const char *ext, bool append)
{
    char fileName[256];
    std::strcpy(fileName, base);
    std::strcat(fileName, ext);
    if (append) {
        return new AppendableFileOutputStream(fileName);
    }
    return new java::FileOutputStream(fileName);
}

int
RawFormat::open(char *name, int *w, int *h, int bufferSize, int openMode, int /*firstLine*/)
{
    mode = openMode;
    filename = name;
    lineCounter = 0;
    redIn = greenIn = blueIn = nullptr;
    redOut = greenOut = blueOut = nullptr;

    switch (mode) {
    case READ_MODE:
        redIn   = RawFormat::openRawInputStream(name, RED_RAW_FILE_EXTENSION);
        greenIn = RawFormat::openRawInputStream(name, GREEN_RAW_FILE_EXTENSION);
        blueIn  = RawFormat::openRawInputStream(name, BLUE_RAW_FILE_EXTENSION);
        if (!redIn || !greenIn || !blueIn) {
            return 0;
        }
        width  = *w;
        height = *h;
        break;

    case WRITE_MODE:
        redOut   = RawFormat::openRawOutputStream(name, RED_RAW_FILE_EXTENSION, false);
        greenOut = RawFormat::openRawOutputStream(name, GREEN_RAW_FILE_EXTENSION, false);
        blueOut  = RawFormat::openRawOutputStream(name, BLUE_RAW_FILE_EXTENSION, false);
        width  = *w;
        height = *h;
        break;

    case APPEND_MODE:
        redOut   = RawFormat::openRawOutputStream(name, RED_RAW_FILE_EXTENSION, true);
        greenOut = RawFormat::openRawOutputStream(name, GREEN_RAW_FILE_EXTENSION, true);
        blueOut  = RawFormat::openRawOutputStream(name, BLUE_RAW_FILE_EXTENSION, true);
        width  = *w;
        height = *h;
        break;
    }
    return 1;
}

void
RawFormat::writeLine(RGBAColor *lineData, int lineNumber)
{
    for (int x = 0; x < width; x++) {
        redOut->write((int)floor(lineData[x].Red * 255.0));
    }
    for (int x = 0; x < width; x++) {
        greenOut->write((int)floor(lineData[x].Green * 255.0));
    }
    for (int x = 0; x < width; x++) {
        blueOut->write((int)floor(lineData[x].Blue * 255.0));
    }

    redOut->flush();
    greenOut->flush();
    blueOut->flush();
}

int
RawFormat::readLine(RGBAColor *lineData, int *lineNumber)
{
    for (int i = 0; i < width; i++) {
        int data = redIn->read();
        if (data == -1) {
            return (i == 0) ? 0 : -1;
        }
        lineData[i].Red = (double)data / 255.0;
    }
    for (int i = 0; i < width; i++) {
        int data = greenIn->read();
        if (data == -1) return -1;
        lineData[i].Green = (double)data / 255.0;
    }
    for (int i = 0; i < width; i++) {
        int data = blueIn->read();
        if (data == -1) return -1;
        lineData[i].Blue = (double)data / 255.0;
    }

    *lineNumber = lineCounter++;
    return 1;
}

void
RawFormat::close()
{
    if (redIn)   { redIn->close();   delete redIn;   redIn   = nullptr; }
    if (greenIn) { greenIn->close(); delete greenIn; greenIn = nullptr; }
    if (blueIn)  { blueIn->close();  delete blueIn;  blueIn  = nullptr; }

    if (redOut)   { redOut->close();   delete redOut;   redOut   = nullptr; }
    if (greenOut) { greenOut->close(); delete greenOut; greenOut = nullptr; }
    if (blueOut)  { blueOut->close();  delete blueOut;  blueOut  = nullptr; }
}
