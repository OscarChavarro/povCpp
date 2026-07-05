
#include "java/lang/Math.h"
#include "java/io/FileOutputStream.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "io/image/TargaFormat.h"

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
TargaFormat::defaultFileName() const
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
        {
            java::File * const located = fileLocator->locate(name);
            if (located == nullptr) {
                return 0;
            }
            inputStream = new java::FileInputStream(located->getPath().toCString());
            delete located;
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
TargaFormat::writeLine(const ColorRgba *lineData, int lineNumber)
{
    for (int x = 0; x < width; x++) {
        outputStream->write((int)java::Math::floor(lineData[x].getB() * 255.0));
        outputStream->write((int)java::Math::floor(lineData[x].getG() * 255.0));
        outputStream->write((int)java::Math::floor(lineData[x].getR() * 255.0));
    }

    outputStream->flush();
}

int
TargaFormat::readLine(ColorRgba *lineData, int *lineNumber)
{
    for (int x = 0; x < width; x++) {
        int data = inputStream->read();
        if (data == -1) {
            return (x == 0) ? 0 : -1;
        }
        lineData[x].setB((double)data / 255.0);

        data = inputStream->read();
        if (data == -1) return -1;
        lineData[x].setG((double)data / 255.0);

        data = inputStream->read();
        if (data == -1) return -1;
        lineData[x].setR((double)data / 255.0);
    }

    return 1;
}

int
TargaFormat::readRow(RGBAImageHDRUncompressed *image, int row)
{
    for (int x = 0; x < width; x++) {
        int raw = inputStream->read();
        if (raw == -1) {
            return (x == 0) ? 0 : -1;
        }
        RGBAPixelHDR pixel;
        pixel.b = (unsigned short)raw;

        raw = inputStream->read();
        if (raw == -1) return -1;
        pixel.g = (unsigned short)raw;

        raw = inputStream->read();
        if (raw == -1) return -1;
        pixel.r = (unsigned short)raw;
        pixel.a = 0;
        image->setPixel(x, row, pixel);
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
TargaFormat::readTargaImage(RGBAImageHDRUncompressed *image, char *name,
    const FileLocator &locator)
{
    TargaFormat fmt;
    fmt.setFileLocator(&locator);
    int w = 0, h = 0;
    if (!fmt.open(name, &w, &h, 0, READ_MODE, 0)) {
        {
            char _logMsg[1024];
            snprintf(_logMsg, sizeof(_logMsg), "Cannot open Targa file %s\n", name);
            Logger::reportMessage("TargaFormat", Logger::FATAL_ERROR, "", _logMsg);
        }
    }

    image->allocate(w, h);

    for (int row = 0; row < image->getYSize() && fmt.readRow(image, row); row++) {
    }

    fmt.close();
}
