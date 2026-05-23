#ifndef __RGBA_IMAGE_H__
#define __RGBA_IMAGE_H__

#include "common/linealAlgebra/Vector3Dd.h"
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
    short onceFlag;
    short useColourFlag;
    Vector3Dd imageGradient;
    short colourMapSize;
    RGBAPixel *Colour_Map;
    ImageData data;
};

#endif
