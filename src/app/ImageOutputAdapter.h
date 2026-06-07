#ifndef __IMAGE_OUTPUT_ADAPTER_H__
#define __IMAGE_OUTPUT_ADAPTER_H__

#include "io/image/ImageOutput.h"
#include "render/RenderOutput.h"

class ImageOutputAdapter : public RenderOutput {
  public:
    explicit ImageOutputAdapter(ImageOutput *delegate);

    const char *defaultFileName() override;
    int open(char *name, int *width, int *height, int bufferSize, int mode,
             int firstLine) override;
    void writeLine(RGBAColor *lineData, int lineNumber) override;
    int readLine(RGBAColor *lineData, int *lineNumber) override;
    void close() override;

  private:
    ImageOutput *delegate;
};

#endif
