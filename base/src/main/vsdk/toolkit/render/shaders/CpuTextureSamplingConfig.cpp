#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/media/Image.h"
#include "vsdk/toolkit/media/NormalMap.h"
#include "vsdk/toolkit/render/shaders/CpuTextureSamplingConfig.h"
double CpuTextureSamplingConfig::textureOffsetUTexels = -0.5;
double CpuTextureSamplingConfig::textureOffsetVTexels = -0.5;
const double CpuTextureSamplingConfig::NORMAL_OFFSET_U_TEXELS = -0.5;
const double CpuTextureSamplingConfig::NORMAL_OFFSET_V_TEXELS = -0.5;

void CpuTextureSamplingConfig::setTextureOffsetTexels(double uTexels, double vTexels)
{
    textureOffsetUTexels = uTexels;
    textureOffsetVTexels = vTexels;
}

ColorRgb CpuTextureSamplingConfig::sample(Image* texture, double u, double v)
{
    if ( texture == 0 ) return ColorRgb();
    double du = texture->getXSize() > 0 ? textureOffsetUTexels / ((double)texture->getXSize()) : 0.0;
    double dv = texture->getYSize() > 0 ? textureOffsetVTexels / ((double)texture->getYSize()) : 0.0;
    ColorRgb* c = texture->getColorRgbBiLinear(u + du, v + dv);
    ColorRgb out = c != 0 ? *c : ColorRgb();
    delete c;
    return out;
}

Vector3Dd CpuTextureSamplingConfig::sampleNormal(NormalMap* normalMap, double u, double v)
{
    if ( normalMap == 0 ) return Vector3Dd();
    double du = normalMap->getXSize() > 0 ? NORMAL_OFFSET_U_TEXELS / ((double)normalMap->getXSize()) : 0.0;
    double dv = normalMap->getYSize() > 0 ? NORMAL_OFFSET_V_TEXELS / ((double)normalMap->getYSize()) : 0.0;
    Vector3Dd* n = normalMap->getNormalBiLinear(u + du, v + dv);
    Vector3Dd out = n != 0 ? *n : Vector3Dd();
    delete n;
    return out;
}
