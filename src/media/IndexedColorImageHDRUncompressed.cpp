#include "media/IndexedColorImageHDRUncompressed.h"

IndexedColorImageHDRUncompressed::IndexedColorImageHDRUncompressed() : xSize(0), ySize(0), data(nullptr), colorMapSize(0), colorMap(nullptr) {
}

IndexedColorImageHDRUncompressed::~IndexedColorImageHDRUncompressed() {
    delete[] data;
}

int
IndexedColorImageHDRUncompressed::getXSize() const {
    return xSize;
}

int
IndexedColorImageHDRUncompressed::getYSize() const {
    return ySize;
}

void
IndexedColorImageHDRUncompressed::setXSize(int w) {
    xSize = w;
}

void
IndexedColorImageHDRUncompressed::setYSize(int h) {
    ySize = h;
}

int
IndexedColorImageHDRUncompressed::getColorMapSize() const {
    return colorMapSize;
}

void
IndexedColorImageHDRUncompressed::setColorMapSize(int n) {
    colorMapSize = n;
}

RGBAPixelHDR *
IndexedColorImageHDRUncompressed::getColorMap() const {
    return colorMap;
}

void
IndexedColorImageHDRUncompressed::setColorMap(RGBAPixelHDR *cm) {
    colorMap = cm;
}

void
IndexedColorImageHDRUncompressed::allocate(int w, int h) {
    xSize = w;
    ySize = h;
    delete[] data;
    data = new unsigned char[w * h]();
}
