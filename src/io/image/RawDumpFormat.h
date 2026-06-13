#ifndef __RAW_DUMP_FORMAT_H__
#define __RAW_DUMP_FORMAT_H__

#include "java/io/FileInputStream.h"
#include "java/io/OutputStream.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "io/image/ImageOutput.h"

class RawDumpFormat : public ImageOutput {
  public:
    RawDumpFormat();
    ~RawDumpFormat() override;
    const char *defaultFileName() const override;
    int open(char *name, int *width, int *height, int bufferSize, int mode,
             int firstLine) override;
    void writeLine(const ColorRgba *lineData, int lineNumber) override;
    int readLine(ColorRgba *lineData, int *lineNumber) override;
    void close() override;
    static void readDumpImage(RGBAImageHDRUncompressed *image, char *name);

  private:
    int readRow(RGBAImageHDRUncompressed *image, int *lineNumber);
    java::FileInputStream *inputStream;
    java::OutputStream *outputStream;
    int width;
    int height;
    int mode;
    char *filename;
};

#endif
