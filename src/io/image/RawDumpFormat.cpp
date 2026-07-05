/**
dump.c

This module contains the code to read and write the dump file format.
The format is as follows:

(header:)
   wwww hhhh         - Width, Height (16 bits, LSB first)

(each scanline:)
   llll                - Line number (16 bits, LSB first)
   rr rr rr ...     - r data for line
   gg gg gg ...     - g data for line
   bb bb bb ...     - b data for line
*/

#include <cstdio>
#include <cstdlib>

#include "java/lang/Math.h"
#include "java/io/FileOutputStream.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/io/PersistenceElement.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/io/FileLocator.h"
#include "io/image/RawDumpFormat.h"

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
RawDumpFormat::defaultFileName() const
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
        {
            java::File * const located = fileLocator->locate(name);
            if (located == nullptr) {
                return 0;
            }
            inputStream = new java::FileInputStream(located->getPath().toCString());
            delete located;
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
RawDumpFormat::writeLine(const ColorRgba *lineData, int lineNumber)
{
    PersistenceElement::writeSignedShortLE(*outputStream, lineNumber);

    for (int x = 0; x < width; x++) {
        outputStream->write((int)java::Math::floor(lineData[x].getR() * 255.0));
    }
    for (int x = 0; x < width; x++) {
        outputStream->write((int)java::Math::floor(lineData[x].getG() * 255.0));
    }
    for (int x = 0; x < width; x++) {
        outputStream->write((int)java::Math::floor(lineData[x].getB() * 255.0));
    }

    outputStream->flush();
}

int
RawDumpFormat::readLine(ColorRgba *lineData, int *lineNumber)
{
    const int lo = inputStream->read();
    if (lo == -1) {
        return 0;
    }
    const int hi = inputStream->read();
    if (hi == -1) {
        return -1;
    }
    *lineNumber = lo + hi * 256;

    for (int i = 0; i < width; i++) {
        const int data = inputStream->read();
        if (data == -1) {
            return -1;
        }
        lineData[i].setR((double)data / 255.0);
    }
    for (int i = 0; i < width; i++) {
        const int data = inputStream->read();
        if (data == -1) {
            return -1;
        }
        lineData[i].setG((double)data / 255.0);
    }
    for (int i = 0; i < width; i++) {
        const int data = inputStream->read();
        if (data == -1) {
            return -1;
        }
        lineData[i].setB((double)data / 255.0);
    }

    return 1;
}

int
RawDumpFormat::readRow(RGBAImageHDRUncompressed *image, int *lineNumber)
{
    const int lo = inputStream->read();
    if (lo == -1) {
        return 0;
    }
    const int hi = inputStream->read();
    if (hi == -1) {
        return -1;
    }
    *lineNumber = lo + hi * 256;

    unsigned char * const rBuf = new unsigned char[width]();
    unsigned char * const gBuf = new unsigned char[width]();
    unsigned char * const bBuf = new unsigned char[width]();

    for (int i = 0; i < width; i++) {
        const int raw = inputStream->read();
        if (raw == -1) { delete[] rBuf; delete[] gBuf; delete[] bBuf; return -1; }
        rBuf[i] = (unsigned char)raw;
    }
    for (int i = 0; i < width; i++) {
        const int raw = inputStream->read();
        if (raw == -1) { delete[] rBuf; delete[] gBuf; delete[] bBuf; return -1; }
        gBuf[i] = (unsigned char)raw;
    }
    for (int i = 0; i < width; i++) {
        const int raw = inputStream->read();
        if (raw == -1) { delete[] rBuf; delete[] gBuf; delete[] bBuf; return -1; }
        bBuf[i] = (unsigned char)raw;
    }

    for (int i = 0; i < width; i++) {
        RGBAPixelHDR pixel;
        pixel.r = rBuf[i];
        pixel.g = gBuf[i];
        pixel.b = bBuf[i];
        pixel.a = 0;
        image->setPixel(i, *lineNumber, pixel);
    }

    delete[] rBuf;
    delete[] gBuf;
    delete[] bBuf;

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
RawDumpFormat::readDumpImage(RGBAImageHDRUncompressed *image, char *name,
    const FileLocator &locator)
{
    RawDumpFormat fmt;
    fmt.setFileLocator(&locator);
    int w = 0, h = 0;
    if (!fmt.open(name, &w, &h, 0, READ_MODE, 0)) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Cannot open dump file %s\n", name);
            Logger::reportMessage("RawDumpFormat", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    image->allocate(w, h);

    int row;
    int rc;
    while ((rc = fmt.readRow(image, &row)) == 1) {
    }

    fmt.close();

    if (rc != 0) {
        exit(1);
    }
}
