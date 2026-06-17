#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"
int
ControlledRGBAImageHDRUncompressed::getMapType() const
{
    return mapType;
}

void
ControlledRGBAImageHDRUncompressed::setMapType(ImageToSolidTextureProjectionMethods v)
{
    mapType = v;
}

void
ControlledRGBAImageHDRUncompressed::setMapType(int v)
{
    switch (v) {
      case 1:
        mapType = ImageToSolidTextureProjectionMethods::SPHERICAL_MAP;
        break;
      case 2:
        mapType = ImageToSolidTextureProjectionMethods::CYLINDRICAL_MAP;
        break;
      case 5:
        mapType = ImageToSolidTextureProjectionMethods::TORUS_MAP;
        break;
      default:
        mapType = ImageToSolidTextureProjectionMethods::PLANAR_MAP;
        break;
    }
}

int
ControlledRGBAImageHDRUncompressed::getInterpolationType() const
{
    return interpolationType;
}

void
ControlledRGBAImageHDRUncompressed::setInterpolationType(ImageToSolidTextureInterpolationTypes v)
{
    interpolationType = v;
}

void
ControlledRGBAImageHDRUncompressed::setInterpolationType(int v)
{
    switch (v) {
      case 1:
        interpolationType = ImageToSolidTextureInterpolationTypes::NEAREST_NEIGHBOR;
        break;
      case 2:
        interpolationType = ImageToSolidTextureInterpolationTypes::BI_LINEAR;
        break;
      case 3:
        interpolationType = ImageToSolidTextureInterpolationTypes::CUBIC_SPLINE;
        break;
      case 4:
        interpolationType = ImageToSolidTextureInterpolationTypes::NORMALIZED_DIST;
        break;
      default:
        interpolationType = ImageToSolidTextureInterpolationTypes::NO_INTERPOLATION;
        break;
    }
}

bool
ControlledRGBAImageHDRUncompressed::getOnceFlag() const
{
    return onceFlag;
}

void
ControlledRGBAImageHDRUncompressed::setOnceFlag(bool v)
{
    onceFlag = v;
}

bool
ControlledRGBAImageHDRUncompressed::getUseColorFlag() const
{
    return useColorFlag;
}

void
ControlledRGBAImageHDRUncompressed::setUseColorFlag(bool v)
{
    useColorFlag = v;
}

const Vector3Dd&
ControlledRGBAImageHDRUncompressed::getImageGradient() const
{
    return imageGradient;
}

void
ControlledRGBAImageHDRUncompressed::setImageGradient(const Vector3Dd& v)
{
    imageGradient = v;
}

IndexedColorImageHDRUncompressed*
ControlledRGBAImageHDRUncompressed::getIndexedData() const
{
    return indexedData;
}

void
ControlledRGBAImageHDRUncompressed::setIndexedData(IndexedColorImageHDRUncompressed* v)
{
    indexedData = v;
}
