#ifndef __RAW_FORMAT_H__
#define __RAW_FORMAT_H__

#include "java/io/FileInputStream.h"
#include "java/io/OutputStream.h"
#include "io/image/ImageOutput.h"

class ColorRgba;

class RawFormat : public ImageOutput {
  public:
    static constexpr const char *RED_RAW_FILE_EXTENSION = ".red";
    static constexpr const char *GREEN_RAW_FILE_EXTENSION = ".grn";
    static constexpr const char *BLUE_RAW_FILE_EXTENSION = ".blu";

    RawFormat();
    ~RawFormat() override;
    const char *defaultFileName() const override;
    int open(char *name, int *width, int *height, int bufferSize, int mode,
             int firstLine) override;
    void writeLine(const ColorRgba *lineData, int lineNumber) override;
    int readLine(ColorRgba *lineData, int *lineNumber) override;
    void close() override;

  private:
    static java::FileInputStream *openRawInputStream(
        const char *base, const char *ext);
    static java::OutputStream *openRawOutputStream(
        const char *base, const char *ext, bool append);

    java::FileInputStream *redIn;
    java::FileInputStream *greenIn;
    java::FileInputStream *blueIn;
    java::OutputStream *redOut;
    java::OutputStream *greenOut;
    java::OutputStream *blueOut;
    int width;
    int height;
    int mode;
    char *filename;
    int lineCounter;
};

#endif
