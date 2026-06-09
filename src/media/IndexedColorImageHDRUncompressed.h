#ifndef __INDEXED_IMAGE_H__
#define __INDEXED_IMAGE_H__

#include "vsdk/toolkit/media/RGBAPixelHDR.h"

class IndexedColorImageHDRUncompressed {
  private:
    int xSize;
    int ySize;
    unsigned char *data;
    int colorMapSize;
    RGBAPixelHDR *colorMap;

  public:
    IndexedColorImageHDRUncompressed();
    virtual ~IndexedColorImageHDRUncompressed();

    int getXSize() const;
    int getYSize() const;
    void setXSize(int w);
    void setYSize(int h);

    int getColorMapSize() const;
    void setColorMapSize(int n);
    RGBAPixelHDR *getColorMap() const;
    void setColorMap(RGBAPixelHDR *cm);

    void allocate(int w, int h);

    unsigned char getPixel(int x, int y) const;
    void setPixel(int x, int y, unsigned char value);
};

inline unsigned char IndexedColorImageHDRUncompressed::getPixel(int x, int y) const {
    return data[y * xSize + x];
}

inline void IndexedColorImageHDRUncompressed::setPixel(int x, int y, unsigned char value) {
    data[y * xSize + x] = value;
}

#endif
