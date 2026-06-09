#include "media/IndexedImage.h"

IndexedImage::IndexedImage() : xSize(0), ySize(0), data(nullptr), colourMapSize(0), colorMap(nullptr) {
}

IndexedImage::~IndexedImage() {
    delete[] data;
}

int
IndexedImage::getXSize() const {
    return xSize;
}

int
IndexedImage::getYSize() const {
    return ySize;
}

void
IndexedImage::setXSize(int w) {
    xSize = w;
}

void
IndexedImage::setYSize(int h) {
    ySize = h;
}

int
IndexedImage::getColourMapSize() const {
    return colourMapSize;
}

void
IndexedImage::setColourMapSize(int n) {
    colourMapSize = n;
}

RGBAPixelHDR *
IndexedImage::getColorMap() const {
    return colorMap;
}

void
IndexedImage::setColorMap(RGBAPixelHDR *cm) {
    colorMap = cm;
}

void
IndexedImage::allocate(int w, int h) {
    xSize = w;
    ySize = h;
    delete[] data;
    data = new unsigned char[w * h]();
}
