#ifndef __TEXTURE_IMAGE_H__
#define __TEXTURE_IMAGE_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageToSolidTextureInterpolationTypes.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ImageToSolidTextureProjectionMethods.h"
class ControlledRGBAImageHDRUncompressed : public RGBAImageHDRUncompressed {
  private:
    ImageToSolidTextureProjectionMethods mapType = ImageToSolidTextureProjectionMethods::PLANAR_MAP;
    ImageToSolidTextureInterpolationTypes interpolationType = ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION;
    bool onceFlag = false;
    bool useColorFlag = true;
    Vector3Dd imageGradient;
    IndexedColorImageHDRUncompressed *indexedData = nullptr;

  public:
    int getMapType() const;
    void setMapType(ImageToSolidTextureProjectionMethods v);
    void setMapType(int v);
    int getInterpolationType() const;
    void setInterpolationType(ImageToSolidTextureInterpolationTypes v);
    void setInterpolationType(int v);
    bool getOnceFlag() const;
    void setOnceFlag(bool v);
    bool getUseColorFlag() const;
    void setUseColorFlag(bool v);
    const Vector3Dd& getImageGradient() const;
    void setImageGradient(const Vector3Dd& v);
    IndexedColorImageHDRUncompressed* getIndexedData() const;
    void setIndexedData(IndexedColorImageHDRUncompressed* v);
};

#endif
