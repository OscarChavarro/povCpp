#ifndef __IMAGE_OUTPUT_ADAPTER__
#define __IMAGE_OUTPUT_ADAPTER__

#include "environment/material/RenderOutput.h"
#include "io/image/ImageOutput.h"

class ImageOutputAdapter : public RenderOutput {
  private:
    ImageOutput * const delegate;

  public:
    explicit ImageOutputAdapter(ImageOutput *delegate);

    int open(char *name, int *width, int *height, int bufferSize, int mode,
             int firstLine) override;
    void writeLine(ColorRgba *lineData, int lineNumber) override;
    void close() override;
};

#endif
