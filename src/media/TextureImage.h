#ifndef __TEXTURE_IMAGE_H__
#define __TEXTURE_IMAGE_H__

#include "media/RGBAImage.h"
#include "media/IndexedImage.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class TextureImage : public RGBAImage {
  public:
    int mapType;
    int interpolationType;
    bool onceFlag;
    bool useColourFlag;
    Vector3Dd imageGradient;
    IndexedImage *indexedData = nullptr;
};

#endif
