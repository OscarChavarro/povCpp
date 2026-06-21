#ifndef __RENDER_IMAGE_WRITER__
#define __RENDER_IMAGE_WRITER__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "render/RenderEngine.h"

class RenderImageWriter {
  private:
    RenderEngine *renderEngine;

  public:
    explicit RenderImageWriter(RenderEngine *engine) : renderEngine(engine) {}
    void readRenderedPart(ColorRgba *lineBuffer);
};

#endif
