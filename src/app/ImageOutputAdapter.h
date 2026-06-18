#ifndef __IMAGE_OUTPUT_ADAPTER_H__
#define __IMAGE_OUTPUT_ADAPTER_H__

#include "environment/material/RenderOutput.h"
#include "io/image/ImageOutput.h"

class ImageOutputAdapter : public RenderOutput {
  private:
    ImageOutput * const delegate;

  public:
    explicit ImageOutputAdapter(ImageOutput *delegate);

    const char *defaultFileName() const override;
    int open(char *name, int *width, int *height, int bufferSize, int mode,
             int firstLine) override;
    void writeLine(ColorRgba *lineData, int lineNumber) override;
    int readLine(ColorRgba *lineData, int *lineNumber) const override;
    void close() override;
};

#endif
