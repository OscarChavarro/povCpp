#ifndef __RGBA_IMAGE_H__
#define __RGBA_IMAGE_H__

#include "media/ImageData.h"
#include "media/RGBAPixel.h"

class RGBAImage {
  public:
    double width;
    double height;
    int iwidth;
    int iheight;
    short colourMapSize;
    RGBAPixel16Bits *colorMap;
    ImageData data;
};

#endif
