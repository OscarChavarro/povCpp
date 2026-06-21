#include "render/RenderTargetImage.h"

RenderTargetImage::RenderTargetImage() : xSize(0), ySize(0), pixels(nullptr) {}

RenderTargetImage::~RenderTargetImage() { delete[] pixels; }

void RenderTargetImage::allocate(int width, int height) {
    delete[] pixels;
    xSize = width;
    ySize = height;
    pixels = new ColorRgba[(long)width * height];
}
