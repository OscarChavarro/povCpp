#ifndef __RGBA_IMAGE_H__
#define __RGBA_IMAGE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "media/ImageData.h"
#include "media/RGBAPixel.h"

class RGBAImage {
  public:
    double width;
    double height;
    int iwidth;
    int iheight;
    int mapType;
    int interpolationType;
    bool onceFlag;
    bool useColourFlag;
    Vector3Dd imageGradient;
    short colourMapSize;
    RGBAPixel *Colour_Map;
    ImageData data;
};

#endif
