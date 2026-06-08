#ifndef __INDEXED_IMAGE_H__
#define __INDEXED_IMAGE_H__

#include "media/RGBAPixel.h"

class IndexedImage {
  public:
    double width;
    double height;
    int iwidth;
    int iheight;
    int colourMapSize;
    RGBAPixel16Bits *colorMap;
    unsigned char **mapLines;
};

#endif
