#ifndef __RGBA_IMAGE_H__
#define __RGBA_IMAGE_H__

#include "common/linealAlgebra/Vector3Dd.h"
#include "media/RGBAPixel.h"
#include "media/ImageData.h"

class RGBAImage {
  public:
    double width, height;
    int iwidth, iheight;
    int Map_Type;
    int Interpolation_Type;
    short Once_Flag;
    short Use_Colour_Flag;
    Vector3Dd Image_Gradient;
    short Colour_Map_Size;
    RGBAPixel *Colour_Map;
    ImageData data;
};

#endif
