#ifndef __RGBAIMAGEHDRUNCOMPRESSED__
#define __RGBAIMAGEHDRUNCOMPRESSED__

#include "vsdk/toolkit/media/RGBAPixelHDR.h"
class RGBAImageHDRUncompressed {
  private:
    int xSize;
    int ySize;
    RGBAPixelHDR *data;

  public:
    RGBAImageHDRUncompressed();
    virtual ~RGBAImageHDRUncompressed();

    int getXSize() const { return xSize; }
    int getYSize() const { return ySize; }

    void allocate(int w, int h);

    void getPixel(int x, int y, RGBAPixelHDR *pixel) const;
    void setPixel(int x, int y, const RGBAPixelHDR &pixel);
};

inline void RGBAImageHDRUncompressed::getPixel(int x, int y, RGBAPixelHDR *pixel) const {
    *pixel = data[y * xSize + x];
}

inline void RGBAImageHDRUncompressed::setPixel(int x, int y, const RGBAPixelHDR &pixel) {
    data[y * xSize + x] = pixel;
}

#endif
