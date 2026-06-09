#ifndef __RGBA_IMAGE_HDR_UNCOMPRESSED_H__
#define __RGBA_IMAGE_HDR_UNCOMPRESSED_H__

#include "media/RGBAPixelHDR.h"

class RGBAImageHDRUncompressed {
  public:
    double width = 0;
    double height = 0;
    int iwidth = 0;
    int iheight = 0;

    RGBAImageHDRUncompressed() = default;
    virtual ~RGBAImageHDRUncompressed() { delete[] data; }

    void allocate(int w, int h) {
        iwidth = w;
        iheight = h;
        width = (double)w;
        height = (double)h;
        delete[] data;
        data = new RGBAPixelHDR[w * h]();
    }

    void getPixel(int x, int y, RGBAPixelHDR *pixel) const {
        *pixel = data[y * iwidth + x];
    }

    void setPixel(int x, int y, const RGBAPixelHDR &pixel) {
        data[y * iwidth + x] = pixel;
    }

  private:
    RGBAPixelHDR *data = nullptr;
};

#endif
