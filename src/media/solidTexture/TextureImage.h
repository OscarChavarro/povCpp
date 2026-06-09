#ifndef __TEXTURE_IMAGE_H__
#define __TEXTURE_IMAGE_H__

#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "media/IndexedImage.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class textureImage : public RGBAImageHDRUncompressed {
  private:
    int mapType = 0;
    int interpolationType = 0;
    bool onceFlag = false;
    bool useColorFlag = true;
    Vector3Dd imageGradient;
    IndexedImage *indexedData = nullptr;

  public:
    int getMapType() const { return mapType; }
    void setMapType(int v) { mapType = v; }

    int getInterpolationType() const { return interpolationType; }
    void setInterpolationType(int v) { interpolationType = v; }

    bool getOnceFlag() const { return onceFlag; }
    void setOnceFlag(bool v) { onceFlag = v; }

    bool getUseColorFlag() const { return useColorFlag; }
    void setUseColorFlag(bool v) { useColorFlag = v; }

    const Vector3Dd& getImageGradient() const { return imageGradient; }
    void setImageGradient(const Vector3Dd& v) { imageGradient = v; }

    IndexedImage* getIndexedData() const { return indexedData; }
    void setIndexedData(IndexedImage* v) { indexedData = v; }
};

#endif
