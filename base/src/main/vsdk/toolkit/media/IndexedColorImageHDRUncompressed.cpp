#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
IndexedColorImageHDRUncompressed::IndexedColorImageHDRUncompressed() : xSize(0), ySize(0), data(nullptr), colorMapSize(0), colorTable(nullptr) {
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
IndexedColorImageHDRUncompressed::getColorTable() const {
    return colorTable;
}

void
IndexedColorImageHDRUncompressed::setColorTable(RGBAPixelHDR *ct) {
    colorTable = ct;
}

void
IndexedColorImageHDRUncompressed::allocate(int w, int h) {
    xSize = w;
    ySize = h;
    delete[] data;
    data = new unsigned char[w * h]();
}
