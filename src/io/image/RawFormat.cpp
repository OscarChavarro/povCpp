/**
raw.c

This module contains the code to read and write the RAW file format.
*/

#include <cstdio>
#include <cstring>

#include "java/lang/Math.h"

#include "java/io/FileOutputStream.h"

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#include "io/image/RawFormat.h"

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
RawFormat::defaultFileName() const
{
    return "data";
}

java::FileInputStream *
RawFormat::openRawInputStream(const char *base, const char *ext)
{
    char fileName[256];
    std::strcpy(fileName, base);
    std::strcat(fileName, ext);
    java::FileInputStream * const s = new java::FileInputStream(fileName);
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
RawFormat::writeLine(const ColorRgba *lineData, int lineNumber)
{
    for (int x = 0; x < width; x++) {
        redOut->write((int)java::Math::floor(lineData[x].getR() * 255.0));
    }
    for (int x = 0; x < width; x++) {
        greenOut->write((int)java::Math::floor(lineData[x].getG() * 255.0));
    }
    for (int x = 0; x < width; x++) {
        blueOut->write((int)java::Math::floor(lineData[x].getB() * 255.0));
    }

    redOut->flush();
    greenOut->flush();
    blueOut->flush();
}

int
RawFormat::readLine(ColorRgba *lineData, int *lineNumber)
{
    for (int i = 0; i < width; i++) {
        const int data = redIn->read();
        if (data == -1) {
            return (i == 0) ? 0 : -1;
        }
        lineData[i].setR((double)data / 255.0);
    }
    for (int i = 0; i < width; i++) {
        const int data = greenIn->read();
        if (data == -1) return -1;
        lineData[i].setG((double)data / 255.0);
    }
    for (int i = 0; i < width; i++) {
        const int data = blueIn->read();
        if (data == -1) return -1;
        lineData[i].setB((double)data / 255.0);
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
