#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "java/util/ArrayList.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/media/RGBAPixelHDR.h"

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "media/solidTexture/TextureImage.h"
#include "media/solidTexture/TextureUtils.h"

class Texture {
  public:
    static constexpr int MINX = -10000; // Ridiculously large scaling offset to ensure positive lattice coords
    static constexpr int MINY = MINX;
    static constexpr int MINZ = MINX;
    static constexpr int MAXSIZE = 267;
    static constexpr long RNDMASK = 0x7FFF;
    static constexpr float rndDivisor = static_cast<float>(RNDMASK);
    static constexpr int NUMBER_OF_WAVES = 10;
    static constexpr int SINTABSIZE = 1000;
    static constexpr double realScale = (2.0 / 65535.0);

    java::ArrayList<Texture*> layers; // Ordered list of additional texture layers
    java::ArrayList<Texture*> materials; // Material map variants
    double objectReflection;
    double objectAmbient;
    double objectDiffuse;
    double objectBrilliance;
    double objectIndexOfRefraction;
    double objectRefraction;
    double objectTransmit;
    double objectSpecular;
    double objectRoughness;
    double objectPhong;
    double objectPhongSize;
    double bumpAmount;
    double textureRandomness;
    double frequency;
    double phase;
    int textureNumber;
    int bumpNumber;
    Matrix4x4d *textureTransformation;
    Matrix4x4d *textureTransformationInverse;
    ColorRgba *color1;
    ColorRgba *color2;
    double turbulence;
    Vector3Dd textureGradient;
    RGBAColorPalette *colorMap;
    textureImage *image;
    textureImage *bumpImage;
    textureImage *materialImage;
    bool metallicFlag;
    bool onceFlag;
    bool constantFlag;
    int octaves; // dmf, 1/92 for turbulence functions
    double mortar; // rha, 2/92 for brick texture
};

#endif
