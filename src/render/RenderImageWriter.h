#ifndef __RENDER_IMAGE_WRITER__
#define __RENDER_IMAGE_WRITER__

#include "vsdk/toolkit/common/color/ColorRgba.h"

class RenderEngine;
class RenderWorker;

// Owns the "image writing" half of the render: persisting completed scanlines
// to the output file and reading them back to resume an interrupted render.
// Kept apart from the sampling driver (RenderEngine) so the driver can be
// swapped (parallel / GPU) without dragging file I/O along.
class RenderImageWriter {
  private:
    RenderEngine *renderEngine;

  public:
    explicit RenderImageWriter(RenderEngine *engine) : renderEngine(engine) {}

    // Resume an interrupted render (CONTINUE_TRACE): read back the scanlines
    // already present in the output file into lineBuffer, then advance the
    // configuration's first line so tracing picks up where it left off.
    void readRenderedPart(ColorRgba *lineBuffer);

    // Persist one completed scanline to the output file when disk output
    // (DISK_WRITE) is enabled; a no-op otherwise.
    void writeScanline(ColorRgba *lineData, int lineNumber);

    // Flush the just-finished scanline: persist the previous line (one behind
    // the cursor, so partially anti-aliased lines are never written) and then
    // recycle the line buffers for the next row.
    void outputLine(RenderWorker &localWorker, int y);
};

#endif
