#ifndef __RAW_FORMAT_H__
#define __RAW_FORMAT_H__

#include "io/image/ImageOutput.h"
#include "java/io/FileInputStream.h"
#include "java/io/FileOutputStream.h"

static constexpr const char *RED_RAW_FILE_EXTENSION   = ".red";
static constexpr const char *GREEN_RAW_FILE_EXTENSION = ".grn";
static constexpr const char *BLUE_RAW_FILE_EXTENSION  = ".blu";

class RGBAColor;

class RawFormat : public ImageOutput {
  public:
    RawFormat();
    ~RawFormat() override;
    const char *defaultFileName() override;
    int open(char *name, int *width, int *height, int bufferSize, int mode) override;
    void writeLine(RGBAColor *lineData, int lineNumber) override;
    int readLine(RGBAColor *lineData, int *lineNumber) override;
    void close() override;

  private:
    java::FileInputStream  *redIn,  *greenIn,  *blueIn;
    java::FileOutputStream *redOut, *greenOut, *blueOut;
    int width, height, mode;
    char *filename;
    int lineCounter;
};

#endif
