#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"

RGBAImageHDRUncompressed::RGBAImageHDRUncompressed() : xSize(0), ySize(0), data(nullptr) {
}

RGBAImageHDRUncompressed::~RGBAImageHDRUncompressed() {
    delete[] data;
}

void RGBAImageHDRUncompressed::allocate(int w, int h) {
    xSize = w;
    ySize = h;
    delete[] data;
    data = new RGBAPixelHDR[w * h]();
}
