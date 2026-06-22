#ifndef __IMAGE_OUTPUT__
#define __IMAGE_OUTPUT__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "io/binaryIo/FileLocator.h"

class ImageOutput {
  protected:
    const FileLocator *fileLocator = nullptr;

  public:
    static constexpr int READ_MODE = 0;
    static constexpr int WRITE_MODE = 1;
    static constexpr int APPEND_MODE = 2;

    virtual ~ImageOutput() {}
    void setFileLocator(const FileLocator *locator) { fileLocator = locator; }
    virtual const char *defaultFileName() const = 0;
    virtual int open(char *name, int *width, int *height, int bufferSize, int mode,
                     int firstLine) = 0;
    virtual void writeLine(const ColorRgba *lineData, int lineNumber) = 0;
    virtual int readLine(ColorRgba *lineData, int *lineNumber) = 0;
    virtual void close() = 0;
};

#endif
