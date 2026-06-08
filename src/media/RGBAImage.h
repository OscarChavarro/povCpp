#ifndef __RGBA_IMAGE_H__
#define __RGBA_IMAGE_H__

#include "media/ImageLine.h"

class RGBAImage {
  public:
    double width;
    double height;
    int iwidth;
    int iheight;
    ImageLine *lines = nullptr;
};

#endif
