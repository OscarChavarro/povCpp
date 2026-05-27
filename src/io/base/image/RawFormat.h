#ifndef __RAW_FORMAT_H__
#define __RAW_FORMAT_H__

#include "io/base/image/ImageOutput.h"
#include "java/io/FileInputStream.h"
#include "java/io/FileOutputStream.h"

class RGBAColor;

class RawFormat : public ImageOutput {
  public:
    static constexpr const char *RED_RAW_FILE_EXTENSION = ".red";
    static constexpr const char *GREEN_RAW_FILE_EXTENSION = ".grn";
    static constexpr const char *BLUE_RAW_FILE_EXTENSION = ".blu";

    RawFormat();
    ~RawFormat() override;
    const char *defaultFileName() override;
    int open(char *name, int *width, int *height, int bufferSize, int mode) override;
    void writeLine(RGBAColor *lineData, int lineNumber) override;
    int readLine(RGBAColor *lineData, int *lineNumber) override;
    void close() override;

  private:
    static java::FileInputStream *openRawInputStream(
        const char *base, const char *ext);
    static java::FileOutputStream *openRawOutputStream(
        const char *base, const char *ext, bool append);

    java::FileInputStream *redIn;
    java::FileInputStream *greenIn;
    java::FileInputStream *blueIn;
    java::FileOutputStream *redOut;
    java::FileOutputStream *greenOut;
    java::FileOutputStream *blueOut;
    int width;
    int height;
    int mode;
    char *filename;
    int lineCounter;
};

#endif
