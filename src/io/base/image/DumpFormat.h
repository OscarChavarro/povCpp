#ifndef __DUMP_FORMAT_H__
#define __DUMP_FORMAT_H__

#include "io/base/image/ImageOutput.h"
#include "media/ImageData.h"
#include "java/io/FileInputStream.h"
#include "java/io/FileOutputStream.h"

class RGBAColor;
class RGBAImage;

class DumpFormat : public ImageOutput {
  public:
    DumpFormat();
    ~DumpFormat() override;
    const char *defaultFileName() override;
    int open(char *name, int *width, int *height, int bufferSize, int mode,
             int firstLine) override;
    void writeLine(RGBAColor *lineData, int lineNumber) override;
    int readLine(RGBAColor *lineData, int *lineNumber) override;
    void close() override;
    static void readDumpImage(RGBAImage *image, char *name);

  private:
    int readIntLine(ImageLine *lineData, int *lineNumber);
    java::FileInputStream *inputStream;
    java::FileOutputStream *outputStream;
    int width;
    int height;
    int mode;
    char *filename;
};

#endif
