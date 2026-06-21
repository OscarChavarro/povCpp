#ifndef __RENDER_TARGET_IMAGE__
#define __RENDER_TARGET_IMAGE__

#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"

// Full-frame, ColorRgba-precision render target. Implements the abstract Image
// interface so RasterTileArea / RasterTileGenerator can refer to it.
class RenderTargetImage : public Image {
  private:
    int xSize;
    int ySize;
    ColorRgba *pixels;   // row-major, ySize * xSize

  public:
    RenderTargetImage();
    ~RenderTargetImage() override;

    void allocate(int width, int height);   // (re)allocate; safe to call again
    bool isAllocated() const { return pixels != nullptr; }

    int getXSize() const override { return xSize; }
    int getYSize() const override { return ySize; }

    ColorRgba *rowPointer(int y) { return &pixels[(long)y * xSize]; }
    const ColorRgba *rowPointer(int y) const { return &pixels[(long)y * xSize]; }
    void setPixel(int x, int y, const ColorRgba &c) { pixels[(long)y * xSize + x] = c; }
};

#endif
