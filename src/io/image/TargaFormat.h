#ifndef __TARGA_FORMAT__
#define __TARGA_FORMAT__

#include "java/io/FileInputStream.h"
#include "java/io/OutputStream.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "io/image/ImageOutput.h"

class TargaFormat : public ImageOutput {
  public:
    TargaFormat();
    ~TargaFormat() override;
    const char *defaultFileName() const override;
    int open(char *name, int *width, int *height, int bufferSize, int mode,
             int firstLine) override;
    void writeLine(const ColorRgba *lineData, int lineNumber) override;
    int readLine(ColorRgba *lineData, int *lineNumber) override;
    void close() override;
    static void readTargaImage(RGBAImageHDRUncompressed *image, char *name,
        const FileLocator &locator);

  private:
    int readRow(RGBAImageHDRUncompressed *image, int row);
    java::FileInputStream *inputStream;
    java::OutputStream *outputStream;
    int width;
    int height;
    int mode;
    char *filename;
};

#endif
