#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "java/util/ArrayList.h"
#include "solidTexture/SolidTextureColorTextures.h"
#include "solidTexture/SolidTextureBumpyTextures.h"
#include "solidTexture/TextureImage.h"
#include "solidTexture/TextureUtils.h"
#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Material {
  public:
    Material();

    java::ArrayList<Material*> layers; // Ordered list of additional texture layers
    java::ArrayList<Material*> materials; // Material map variants
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
    SolidTextureColorTextures textureNumber;
    SolidTextureBumpyTextures bumpNumber;
    Matrix4x4d *textureTransformation;
    Matrix4x4d *textureTransformationInverse;
    ColorRgba *color1;
    ColorRgba *color2;
    double turbulence;
    Vector3Dd textureGradient;
    RGBAColorPalette *colorMap;
    TextureImage *image;
    TextureImage *bumpImage;
    TextureImage *materialImage;
    bool metallicFlag;
    bool onceFlag;
    bool constantFlag;
    int octaves; // dmf, 1/92 for turbulence functions
    double mortar; // rha, 2/92 for brick texture
};

#endif
