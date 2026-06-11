#ifndef __TEXTURE_IMAGE_H__
#define __TEXTURE_IMAGE_H__

#include "ImageToSolidTextureProjectionMethods.h"
#include "solidTexture/from2d/ImageToSolidTextureInterpolationTypes.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/RGBAImageHDRUncompressed.h"

class ControlledRGBAImageHDRUncompressed : public RGBAImageHDRUncompressed {
  private:
    ImageToSolidTextureProjectionMethods mapType = ImageToSolidTextureProjectionMethods::PLANAR_MAP;
    ImageToSolidTextureInterpolationTypes interpolationType = ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION;
    bool onceFlag = false;
    bool useColorFlag = true;
    Vector3Dd imageGradient;
    IndexedColorImageHDRUncompressed *indexedData = nullptr;

  public:
    int getMapType() const { return mapType; }
    void setMapType(ImageToSolidTextureProjectionMethods v) { mapType = v; }
    void setMapType(int v) {
        switch (v) {
        case 1: mapType = ImageToSolidTextureProjectionMethods::SPHERICAL_MAP; break;
        case 2: mapType = ImageToSolidTextureProjectionMethods::CYLINDRICAL_MAP; break;
        case 5: mapType = ImageToSolidTextureProjectionMethods::TORUS_MAP; break;
        default: mapType = ImageToSolidTextureProjectionMethods::PLANAR_MAP; break;
        }
    }

    int getInterpolationType() const { return interpolationType; }
    void setInterpolationType(ImageToSolidTextureInterpolationTypes v) { interpolationType = v; }

    void setInterpolationType(int v)
    {
        switch (v) {
            case 1: interpolationType = ImageToSolidTextureInterpolationTypes::NEAREST_NEIGHBOR; break;
            case 2: interpolationType = ImageToSolidTextureInterpolationTypes::BI_LINEAR; break;
            case 3: interpolationType = ImageToSolidTextureInterpolationTypes::CUBIC_SPLINE; break;
            case 4: interpolationType = ImageToSolidTextureInterpolationTypes::NORMALIZED_DIST; break;
            default: interpolationType = ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION; break;
        }
    }

    bool getOnceFlag() const { return onceFlag; }
    void setOnceFlag(bool v) { onceFlag = v; }

    bool getUseColorFlag() const { return useColorFlag; }
    void setUseColorFlag(bool v) { useColorFlag = v; }

    const Vector3Dd& getImageGradient() const { return imageGradient; }
    void setImageGradient(const Vector3Dd& v) { imageGradient = v; }

    IndexedColorImageHDRUncompressed* getIndexedData() const { return indexedData; }
    void setIndexedData(IndexedColorImageHDRUncompressed* v) { indexedData = v; }
};

#endif
