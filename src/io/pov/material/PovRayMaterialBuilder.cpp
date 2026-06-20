#include "io/pov/material/PovRayMaterialBuilder.h"
#include "environment/material/SolidTextureColorNames.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "java/util/ArrayList.txx"

PovRayMaterialBuilder::PovRayMaterialBuilder() :
    bumpAmount(0.0),
    bumpImage(nullptr),
    bumpNumber(SolidTextureBumpyNames::NO_BUMPS),
    color1(nullptr),
    color2(nullptr),
    colorMap(nullptr),
    frequency(1.0),
    image(nullptr),
    materialImage(nullptr),
    metallicFlag(false),
    mortar(0.2),
    numberOfWaves(PovRayMaterial::DEFAULT_NUMBER_OF_WAVES),
    objectAmbient(0.1),
    objectBrilliance(1.0),
    objectDiffuse(0.6),
    objectIndexOfRefraction(1.0),
    objectPhong(0.0),
    objectPhongSize(40),
    objectReflection(0.0),
    objectRefraction(0.0),
    objectRoughness(0.05),
    objectSpecular(0.0),
    objectTransmit(0.0),
    octaves(6),
    phase(0.0),
    textureGradient(0.0, 0.0, 0.0),
    textureNumber(SolidTextureColorNames::NO_TEXTURE),
    textureRandomness(0.0),
    textureTransformation(nullptr),
    textureTransformationInverse(nullptr),
    turbulence(0.0)
{
}

PovRayMaterialBuilder::PovRayMaterialBuilder(const PovRayMaterial *base) :
    bumpAmount(base->getBumpAmount()),
    bumpImage(base->getBumpImage()),
    bumpNumber(base->getBumpPatternType()),
    color1(base->getColor1()),
    color2(base->getColor2()),
    colorMap(deepCopyColorMap(base->getColorMap())),
    frequency(base->getBumpFrequency()),
    image(base->getColorImage()),
    materialImage(base->getMaterialMapImage()),
    metallicFlag(base->isMetallic()),
    mortar(base->getBrickMortar()),
    numberOfWaves(base->getBumpNumberOfWaves()),
    objectAmbient(base->getObjectAmbient()),
    objectBrilliance(base->getObjectBrilliance()),
    objectDiffuse(base->getObjectDiffuse()),
    objectIndexOfRefraction(base->getObjectIndexOfRefraction()),
    objectPhong(base->getObjectPhong()),
    objectPhongSize(base->getObjectPhongSize()),
    objectReflection(base->getObjectReflection()),
    objectRefraction(base->getObjectRefraction()),
    objectRoughness(base->getObjectRoughness()),
    objectSpecular(base->getObjectSpecular()),
    objectTransmit(base->getObjectTransmit()),
    octaves(base->getOctaves()),
    phase(base->getBumpPhase()),
    textureGradient(base->getTextureGradient()),
    textureNumber(base->getColorPatternType()),
    textureRandomness(base->getTextureRandomness()),
    textureTransformation(nullptr),
    textureTransformationInverse(nullptr),
    turbulence(base->getTurbulence())
{
    if (base->getTextureTransformation() != nullptr) {
        textureTransformation = new Matrix4x4d(*base->getTextureTransformation());
    }
    if (base->getTextureTransformationInverse() != nullptr) {
        textureTransformationInverse = new Matrix4x4d(*base->getTextureTransformationInverse());
    }

    for (long int i = 0; i < base->getLayers().size(); i++) {
        layers.add(copyTextureNode(base->getLayers()[i]));
    }
    for (long int i = 0; i < base->getMaterialMapVariants().size(); i++) {
        materials.add(base->getMaterialMapVariants()[i]);
    }
}

PovRayMaterial *
PovRayMaterialBuilder::build() const
{
    return new PovRayMaterial(objectReflection, objectAmbient, objectDiffuse,
        objectBrilliance, objectIndexOfRefraction, objectRefraction,
        objectTransmit, objectSpecular, objectRoughness, objectPhong,
        objectPhongSize, bumpAmount, textureRandomness, frequency, phase,
        textureNumber, bumpNumber, textureTransformation,
        textureTransformationInverse, color1, color2, turbulence,
        textureGradient, colorMap, image, bumpImage, materialImage,
        metallicFlag, numberOfWaves, octaves, mortar, layers, materials);
}

PovRayMaterial *
PovRayMaterialBuilder::copyTextureNode(const PovRayMaterial *src)
{
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    if (src->getTextureTransformation() != nullptr) {
        transformation = new Matrix4x4d(*src->getTextureTransformation());
        transformationInverse = new Matrix4x4d(*src->getTextureTransformationInverse());
    }
    return new PovRayMaterial(
        src->getObjectReflection(), src->getObjectAmbient(), src->getObjectDiffuse(),
        src->getObjectBrilliance(), src->getObjectIndexOfRefraction(), src->getObjectRefraction(),
        src->getObjectTransmit(), src->getObjectSpecular(), src->getObjectRoughness(),
        src->getObjectPhong(), src->getObjectPhongSize(), src->getBumpAmount(),
        src->getTextureRandomness(), src->getBumpFrequency(),
        src->getBumpPhase(), src->getColorPatternType(),
        src->getBumpPatternType(),
        transformation, transformationInverse,
        src->getColor1(), src->getColor2(), src->getTurbulence(),
        src->getTextureGradient(), deepCopyColorMap(src->getColorMap()),
        src->getColorImage(), src->getBumpImage(), src->getMaterialMapImage(), src->isMetallic(),
        src->getBumpNumberOfWaves(), src->getOctaves(), src->getBrickMortar(),
        src->getLayers(), src->getMaterialMapVariants());
}

// Deep-copies a color map by reading each stored color and adding it to a new palette.
RGBAColorPalette *
PovRayMaterialBuilder::deepCopyColorMap(const RGBAColorPalette *source)
{
    if (source == nullptr) {
        return nullptr;
    }
    RGBAColorPalette * const newMap = new RGBAColorPalette();
    for (int i = 0; i < source->size(); i++) {
        const ColorRgba *c = source->getColorAt(i);
        if (source->hasPositions()) {
            newMap->addColorAt(source->getPositionAt(i), *c);
        } else {
            newMap->addColor(*c);
        }
        delete c;
    }
    return newMap;
}

ColorRgba *
PovRayMaterialBuilder::getColor1() const
{
    return color1;
}

ColorRgba *
PovRayMaterialBuilder::getColor2() const
{
    return color2;
}

ControlledRGBAImageHDRUncompressed *
PovRayMaterialBuilder::getMaterialImage() const
{
    return materialImage;
}

java::ArrayList<PovRayMaterial *> &
PovRayMaterialBuilder::getMaterials()
{
    return materials;
}

double
PovRayMaterialBuilder::getMortar() const
{
    return mortar;
}

int
PovRayMaterialBuilder::getOctaves() const
{
    return octaves;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::rotate(Vector3Dd &v)
{
    if ((textureNumber != SolidTextureColorNames::NO_TEXTURE &&
            textureNumber != SolidTextureColorNames::COLOUR_TEXTURE) ||
        bumpNumber != SolidTextureBumpyNames::NO_BUMPS) {

        if (!textureTransformation) {
            textureTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
            textureTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
        }

        Matrix4x4d deltaTransformation;
        Matrix4x4d deltaTransformationInverse;
        deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, &v);

        *textureTransformation =
            textureTransformation->multiply(deltaTransformation);
        *textureTransformationInverse = deltaTransformationInverse.multiply(
            *textureTransformationInverse);
    }
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::scale(const Vector3Dd &v)
{
    if ((textureNumber != SolidTextureColorNames::NO_TEXTURE &&
            textureNumber != SolidTextureColorNames::COLOUR_TEXTURE) ||
        bumpNumber != SolidTextureBumpyNames::NO_BUMPS) {

        if (!textureTransformation) {
            textureTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
            textureTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
        }

        Matrix4x4d deltaTransformation = Matrix4x4d().scale(
            v.x(), v.y(), v.z());
        Matrix4x4d deltaTransformationInverse = Matrix4x4d().scale(
            1.0 / v.x(), 1.0 / v.y(), 1.0 / v.z());

        *textureTransformation =
            textureTransformation->multiply(deltaTransformation);
        *textureTransformationInverse = deltaTransformationInverse.multiply(
            *textureTransformationInverse);
    }
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setBumpAmount(double v)
{
    bumpAmount = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setBumpImage(ControlledRGBAImageHDRUncompressed *v)
{
    bumpImage = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setBumpNumber(SolidTextureBumpyNames v)
{
    bumpNumber = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setColor1(ColorRgba *v)
{
    color1 = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setColor2(ColorRgba *v)
{
    color2 = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setColorMap(RGBAColorPalette *v)
{
    colorMap = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setFrequency(double v)
{
    frequency = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setImage(ControlledRGBAImageHDRUncompressed *v)
{
    image = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setMaterialImage(ControlledRGBAImageHDRUncompressed *v)
{
    materialImage = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setMetallicFlag(bool v)
{
    metallicFlag = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setMortar(double v)
{
    mortar = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectAmbient(double v)
{
    objectAmbient = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectBrilliance(double v)
{
    objectBrilliance = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectDiffuse(double v)
{
    objectDiffuse = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectIndexOfRefraction(double v)
{
    objectIndexOfRefraction = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectPhong(double v)
{
    objectPhong = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectPhongSize(double v)
{
    objectPhongSize = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectReflection(double v)
{
    objectReflection = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectRefraction(double v)
{
    objectRefraction = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectRoughness(double v)
{
    objectRoughness = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectSpecular(double v)
{
    objectSpecular = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectTransmit(double v)
{
    objectTransmit = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setOctaves(int v)
{
    octaves = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setPhase(double v)
{
    phase = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTextureGradient(const Vector3Dd &v)
{
    textureGradient = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTextureNumber(SolidTextureColorNames v)
{
    textureNumber = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTextureRandomness(double v)
{
    textureRandomness = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTurbulence(double v)
{
    turbulence = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::translate(const Vector3Dd &v)
{
    if ((textureNumber != SolidTextureColorNames::NO_TEXTURE &&
            textureNumber != SolidTextureColorNames::COLOUR_TEXTURE) ||
        bumpNumber != SolidTextureBumpyNames::NO_BUMPS) {

        if (!textureTransformation) {
            textureTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
            textureTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
        }

        Matrix4x4d deltaTransformation = Matrix4x4d().translation(
            v.x(), v.y(), v.z()).transpose();
        Matrix4x4d deltaTransformationInverse = Matrix4x4d().translation(
            0.0 - v.x(), 0.0 - v.y(), 0.0 - v.z()).transpose();

        *textureTransformation =
            textureTransformation->multiply(deltaTransformation);
        *textureTransformationInverse = deltaTransformationInverse.multiply(
            *textureTransformationInverse);
    }
    return *this;
}
