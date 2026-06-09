#ifndef __INDEXED_IMAGE_H__
#define __INDEXED_IMAGE_H__

#include "media/RGBAPixelHDR.h"

class IndexedImage {
  public:
    double width;
    double height;
    int iwidth;
    int iheight;
    int colourMapSize;
    RGBAPixelHDR *colorMap;
    unsigned char **mapLines;
};

#endif
