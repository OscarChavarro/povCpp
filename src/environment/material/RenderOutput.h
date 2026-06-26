#ifndef __RENDER_OUTPUT__
#define __RENDER_OUTPUT__

#include "vsdk/toolkit/common/color/ColorRgba.h"

class RenderOutput {
  public:
    static constexpr int READ_MODE = 0;
    static constexpr int WRITE_MODE = 1;
    static constexpr int APPEND_MODE = 2;

    virtual ~RenderOutput() {}
    virtual const char *defaultFileName() const { return nullptr; }
    virtual int open(char *name, int *width, int *height, int bufferSize, int mode,
                     int firstLine) = 0;
    virtual void writeLine(ColorRgba *lineData, int lineNumber) = 0;
    virtual int readLine(ColorRgba *lineData, int *lineNumber) const { return 0; }
    virtual void close() = 0;
};

#endif
