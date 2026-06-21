#include "render/RenderTargetImage.h"

RenderTargetImage::RenderTargetImage() : xSize(0), ySize(0), pixels(nullptr) {}

RenderTargetImage::~RenderTargetImage() { delete[] pixels; }

void RenderTargetImage::allocate(int width, int height) {
    delete[] pixels;
    xSize = width;
    ySize = height;
    pixels = new ColorRgba[(long)width * height];
}

bool RenderTargetImage::init(int width, int height) {
    allocate(width, height);
    return true;
}

bool RenderTargetImage::initNoFill(int width, int height) {
    allocate(width, height);
    return true;
}

void RenderTargetImage::putPixelRgb(int x, int y, RGBPixel* p) {
    double r = (double)(unsigned char)p->getR() / 255.0;
    double g = (double)(unsigned char)p->getG() / 255.0;
    double b = (double)(unsigned char)p->getB() / 255.0;
    setPixel(x, y, ColorRgba(r, g, b, 1.0));
}

RGBPixel* RenderTargetImage::getPixelRgb(int x, int y) const {
    RGBPixel* p = new RGBPixel();
    getPixelRgb(x, y, p);
    return p;
}

void RenderTargetImage::getPixelRgb(int x, int y, RGBPixel* p) const {
    const ColorRgba &c = pixels[(long)y * xSize + x];
    p->setR((char)((int)(c.getR() * 255.0)));
    p->setG((char)((int)(c.getG() * 255.0)));
    p->setB((char)((int)(c.getB() * 255.0)));
}
