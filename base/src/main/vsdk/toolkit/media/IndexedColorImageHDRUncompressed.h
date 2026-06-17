#ifndef __VSDK_TOOLKIT_MEDIA_INDEXEDCOLORIMAGEHDRUNCOMPRESSED_H__
#define __VSDK_TOOLKIT_MEDIA_INDEXEDCOLORIMAGEHDRUNCOMPRESSED_H__

#include "vsdk/toolkit/media/RGBAPixelHDR.h"
class IndexedColorImageHDRUncompressed {
  private:
    unsigned char *data;
    int xSize;
    int ySize;
    int colorMapSize;
    RGBAPixelHDR *colorTable;

  public:
    IndexedColorImageHDRUncompressed();
    virtual ~IndexedColorImageHDRUncompressed();

    int getXSize() const;
    int getYSize() const;
    void setXSize(int w);
    void setYSize(int h);

    int getColorMapSize() const;
    void setColorMapSize(int n);
    RGBAPixelHDR *getColorTable() const;
    void setColorTable(RGBAPixelHDR *ct);

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
