#ifndef __INDEXED_IMAGE_H__
#define __INDEXED_IMAGE_H__

#include "media/RGBAPixelHDR.h"

class IndexedImage {
  private:
    unsigned char *data = nullptr;

public:
    double width = 0;
    double height = 0;
    int iwidth = 0;
    int iheight = 0;
    int colourMapSize = 0;
    RGBAPixelHDR *colorMap = nullptr;

    IndexedImage() {}
    virtual ~IndexedImage() { delete[] data; }

    void allocate(int w, int h) {
        iwidth = w;
        iheight = h;
        width = (double)w;
        height = (double)h;
        delete[] data;
        data = new unsigned char[w * h]();
    }

    unsigned char getPixel(int x, int y) const {
        return data[y * iwidth + x];
    }

    void setPixel(int x, int y, unsigned char value) {
        data[y * iwidth + x] = value;
    }
};

#endif
