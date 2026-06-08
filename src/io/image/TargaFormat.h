#ifndef __TARGA_FORMAT_H__
#define __TARGA_FORMAT_H__

#include "io/image/ImageOutput.h"
#include "media/ImageLine.h"
#include "java/io/FileInputStream.h"
#include "java/io/OutputStream.h"

class RGBAColor;
class RGBAImage;

class TargaFormat : public ImageOutput {
  public:
    TargaFormat();
    ~TargaFormat() override;
    const char *defaultFileName() override;
    int open(char *name, int *width, int *height, int bufferSize, int mode,
             int firstLine) override;
    void writeLine(RGBAColor *lineData, int lineNumber) override;
    int readLine(RGBAColor *lineData, int *lineNumber) override;
    void close() override;
    static void readTargaImage(RGBAImage *image, char *name);

  private:
    int readIntLine(ImageLine *lineData);
    java::FileInputStream *inputStream;
    java::OutputStream *outputStream;
    int width;
    int height;
    int mode;
    char *filename;
};

#endif
