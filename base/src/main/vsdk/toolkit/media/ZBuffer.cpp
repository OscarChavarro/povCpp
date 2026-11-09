#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/media/IndexedColorImageUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageUncompressed.h"
#include "vsdk/toolkit/media/RGBColorPalette.h"
#include "vsdk/toolkit/media/RGBImageUncompressed.h"
#include "vsdk/toolkit/media/ZBuffer.h"
ZBuffer::ZBuffer(int width, int height) : xSize(width), ySize(height) {
    depth = new float[xSize * ySize];
    for (int i = 0; i < xSize * ySize; i++) {
        depth[i] = 0.0f;
    }
}

ZBuffer::ZBuffer(float* dep, int width, int height) : xSize(width), ySize(height) {
    depth = new float[xSize * ySize];

    int pos = 0;
    for (int y = ySize - 1; y >= 0; y--) {
        for (int x = 0; x < xSize; x++) {
            depth[xSize * y + x] = dep[pos];
            pos++;
        }
    }
}

ZBuffer::~ZBuffer() {
    if (depth != nullptr) {
        delete[] depth;
        depth = nullptr;
    }
}

int ZBuffer::getXSize() const {
    return xSize;
}

int ZBuffer::getYSize() const {
    return ySize;
}

float* ZBuffer::getZBuffer() const {
    if (depth == nullptr) {
        return nullptr;
    }
    float* copy = new float[xSize * ySize];
    for (int i = 0; i < xSize * ySize; i++) {
        copy[i] = depth[i];
    }
    return copy;
}

float ZBuffer::getDepth(int x, int y) const {
    if (x < 0 || x >= xSize || y < 0 || y >= ySize) {
        return 0.0f;
    }
    return depth[y * xSize + x];
}

void ZBuffer::setDepth(int x, int y, float value) {
    if (x >= 0 && x < xSize && y >= 0 && y < ySize) {
        depth[y * xSize + x] = value;
    }
}

ZBuffer* ZBuffer::clone() const {
    ZBuffer* copy = new ZBuffer(xSize, ySize);
    for (int i = 0; i < xSize * ySize; i++) {
        copy->depth[i] = depth[i];
    }
    return copy;
}

IndexedColorImageUncompressed* ZBuffer::exportIndexedColorImage() const
{
    IndexedColorImageUncompressed* image = new IndexedColorImageUncompressed();
    image->init(xSize, ySize);
    int pos = 0;
    int val;

    for ( int y = 0; y < image->getYSize(); y++ ) {
        for ( int x = 0; x < image->getXSize(); x++ ) {
            float f = depth[pos];
            if ( f < 0.0 ) f = 0.0f;
            if ( f > 1.0 ) f = 1.0f;
            val = (int)(f * 255.0);
            image->putPixel(x, y, (char)(val & 0xFF));
            pos++;
        }
    }
    return image;
}

RGBImageUncompressed* ZBuffer::exportRGBImage(const RGBColorPalette* p) const
{
    RGBImageUncompressed* image = new RGBImageUncompressed();
    image->init(xSize, ySize);
    int pos = 0;

    for ( int y = 0; y < image->getYSize(); y++ ) {
        for ( int x = 0; x < image->getXSize(); x++ ) {
            float f = depth[pos];
            if ( f < 0.0 ) f = 0.0f;
            if ( f > 1.0 ) f = 1.0f;
            ColorRgb* c = p->evalLinear(f);
            image->putPixel(x, y,
                (char)(int)(c->r()*256), (char)(int)(c->g()*256),
                (char)(int)(c->b()*256));
            delete c;
            pos++;
        }
    }
    return image;
}

RGBAImageUncompressed* ZBuffer::exportRGBAImage(const RGBColorPalette* p) const
{
    RGBAImageUncompressed* image = new RGBAImageUncompressed();
    image->init(xSize, ySize);
    int pos = 0;

    for ( int y = 0; y < image->getYSize(); y++ ) {
        for ( int x = 0; x < image->getXSize(); x++ ) {
            float f = depth[pos];
            if ( f < 0.0 ) f = 0.0f;
            if ( f > 1.0 ) f = 1.0f;
            ColorRgb* c = p->evalLinear(f);
            // Opaque, as done by the Java version
            image->putPixelA(x, y,
                (char)(int)(c->r()*256), (char)(int)(c->g()*256),
                (char)(int)(c->b()*256), (char)(unsigned char)255);
            delete c;
            pos++;
        }
    }
    return image;
}
