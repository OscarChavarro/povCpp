#include "environment/material/PovrayMaterial.h"

#include "java/util/ArrayList.txx"
#include "environment/material/PovrayMaterialUtils.h"

PovrayMaterial::PovrayMaterial() :
    objectReflection(0.0),
    objectAmbient(0.1),
    objectDiffuse(0.6),
    objectBrilliance(1.0),
    objectIndexOfRefraction(1.0),
    objectRefraction(0.0),
    objectTransmit(0.0),
    objectSpecular(0.0),
    objectRoughness(0.05),
    objectPhong(0.0),
    objectPhongSize(40),
    bumpAmount(0.0),
    textureRandomness(0.0),
    frequency(1.0),
    phase(0.0),
    textureNumber(SolidTextureColorNames::NO_TEXTURE),
    bumpNumber(SolidTextureBumpyNames::NO_BUMPS),
    textureTransformation(nullptr),
    textureTransformationInverse(nullptr),
    color1(nullptr),
    color2(nullptr),
    turbulence(0.0),
    textureGradient(0.0, 0.0, 0.0),
    colorMap(nullptr),
    image(nullptr),
    bumpImage(nullptr),
    materialImage(nullptr),
    metallicFlag(false),
    onceFlag(false),
    constantFlag(true),
    numberOfWaves(DEFAULT_NUMBER_OF_WAVES),
    octaves(6),
    mortar(0.2)
{
}

PovrayMaterial *
PovrayMaterial::copy()
{
    return PovrayMaterialUtils::copyTexture(this);
}

void
PovrayMaterial::translate(Vector3Dd *vector)
{
    PovrayMaterial *self = this;
    PovrayMaterialUtils::translateTexture(&self, vector);
}

void
PovrayMaterial::rotate(Vector3Dd *vector)
{
    PovrayMaterial *self = this;
    PovrayMaterialUtils::rotateTexture(&self, vector);
}

void
PovrayMaterial::scale(Vector3Dd *vector)
{
    PovrayMaterial *self = this;
    PovrayMaterialUtils::scaleTexture(&self, vector);
}
