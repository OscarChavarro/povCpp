#ifndef __RENDER_OUTPUT_H__
#define __RENDER_OUTPUT_H__

class RGBAColor;

class RenderOutput {
  public:
    static constexpr int READ_MODE = 0;
    static constexpr int WRITE_MODE = 1;
    static constexpr int APPEND_MODE = 2;

    virtual ~RenderOutput() {}
    virtual const char *defaultFileName() = 0;
    virtual int open(char *name, int *width, int *height, int bufferSize, int mode) = 0;
    virtual void writeLine(RGBAColor *lineData, int lineNumber) = 0;
    virtual int readLine(RGBAColor *lineData, int *lineNumber) = 0;
    virtual void close() = 0;
};

#endif
