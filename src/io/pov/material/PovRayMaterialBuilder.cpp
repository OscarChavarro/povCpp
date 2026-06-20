#include "io/pov/material/PovRayMaterialBuilder.h"
#include "environment/material/SolidTextureColorNames.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "java/util/ArrayList.txx"

namespace {

// Deep-copies a color map the same way PovRayMaterialUtils::copyTextureNode does:
// each stored color is read out (getColorAt returns a fresh allocation) and re-added.
RGBAColorPalette *
deepCopyColorMap(const RGBAColorPalette *source)
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

// Produces a fresh PovRayMaterial mirroring a single node the way copyTexture does:
// every field is shallow-copied except the transformation matrices and the color map,
// which are deep-copied. Sub-layers are carried over shallow (as copyTextureNode leaves
// them), and the resulting node is non-constant.
PovRayMaterial *
copyTextureNode(const PovRayMaterial *src)
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

} // namespace

PovRayMaterialBuilder::PovRayMaterialBuilder() :
    objectReflection_(0.0),
    objectAmbient_(0.1),
    objectDiffuse_(0.6),
    objectBrilliance_(1.0),
    objectIndexOfRefraction_(1.0),
    objectRefraction_(0.0),
    objectTransmit_(0.0),
    objectSpecular_(0.0),
    objectRoughness_(0.05),
    objectPhong_(0.0),
    objectPhongSize_(40),
    bumpAmount_(0.0),
    textureRandomness_(0.0),
    frequency_(1.0),
    phase_(0.0),
    textureNumber_(SolidTextureColorNames::NO_TEXTURE),
    bumpNumber_(SolidTextureBumpyNames::NO_BUMPS),
    textureTransformation_(nullptr),
    textureTransformationInverse_(nullptr),
    color1_(nullptr),
    color2_(nullptr),
    turbulence_(0.0),
    textureGradient_(0.0, 0.0, 0.0),
    colorMap_(nullptr),
    image_(nullptr),
    bumpImage_(nullptr),
    materialImage_(nullptr),
    metallicFlag_(false),
    numberOfWaves_(PovRayMaterial::DEFAULT_NUMBER_OF_WAVES),
    octaves_(6),
    mortar_(0.2)
{
}

PovRayMaterialBuilder::PovRayMaterialBuilder(const PovRayMaterial *base) :
    objectReflection_(base->getObjectReflection()),
    objectAmbient_(base->getObjectAmbient()),
    objectDiffuse_(base->getObjectDiffuse()),
    objectBrilliance_(base->getObjectBrilliance()),
    objectIndexOfRefraction_(base->getObjectIndexOfRefraction()),
    objectRefraction_(base->getObjectRefraction()),
    objectTransmit_(base->getObjectTransmit()),
    objectSpecular_(base->getObjectSpecular()),
    objectRoughness_(base->getObjectRoughness()),
    objectPhong_(base->getObjectPhong()),
    objectPhongSize_(base->getObjectPhongSize()),
    bumpAmount_(base->getBumpAmount()),
    textureRandomness_(base->getTextureRandomness()),
    frequency_(base->getBumpFrequency()),
    phase_(base->getBumpPhase()),
    textureNumber_(base->getColorPatternType()),
    bumpNumber_(base->getBumpPatternType()),
    textureTransformation_(nullptr),
    textureTransformationInverse_(nullptr),
    color1_(base->getColor1()),
    color2_(base->getColor2()),
    turbulence_(base->getTurbulence()),
    textureGradient_(base->getTextureGradient()),
    colorMap_(deepCopyColorMap(base->getColorMap())),
    image_(base->getColorImage()),
    bumpImage_(base->getBumpImage()),
    materialImage_(base->getMaterialMapImage()),
    metallicFlag_(base->isMetallic()),
    numberOfWaves_(base->getBumpNumberOfWaves()),
    octaves_(base->getOctaves()),
    mortar_(base->getBrickMortar())
{
    if (base->getTextureTransformation() != nullptr) {
        textureTransformation_ = new Matrix4x4d(*base->getTextureTransformation());
    }
    if (base->getTextureTransformationInverse() != nullptr) {
        textureTransformationInverse_ = new Matrix4x4d(*base->getTextureTransformationInverse());
    }

    for (long int i = 0; i < base->getLayers().size(); i++) {
        layers_.add(copyTextureNode(base->getLayers()[i]));
    }
    for (long int i = 0; i < base->getMaterialMapVariants().size(); i++) {
        materials_.add(base->getMaterialMapVariants()[i]);
    }
}

PovRayMaterial *
PovRayMaterialBuilder::build() const
{
    return new PovRayMaterial(
        objectReflection_, objectAmbient_, objectDiffuse_,
        objectBrilliance_, objectIndexOfRefraction_, objectRefraction_,
        objectTransmit_, objectSpecular_, objectRoughness_,
        objectPhong_, objectPhongSize_, bumpAmount_,
        textureRandomness_, frequency_, phase_,
        textureNumber_, bumpNumber_,
        textureTransformation_, textureTransformationInverse_,
        color1_, color2_, turbulence_,
        textureGradient_, colorMap_,
        image_, bumpImage_, materialImage_, metallicFlag_,
        numberOfWaves_, octaves_, mortar_,
        layers_, materials_);
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectReflection(double v)
{
    objectReflection_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectAmbient(double v)
{
    objectAmbient_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectDiffuse(double v)
{
    objectDiffuse_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectBrilliance(double v)
{
    objectBrilliance_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectIndexOfRefraction(double v)
{
    objectIndexOfRefraction_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectRefraction(double v)
{
    objectRefraction_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectTransmit(double v)
{
    objectTransmit_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectSpecular(double v)
{
    objectSpecular_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectRoughness(double v)
{
    objectRoughness_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectPhong(double v)
{
    objectPhong_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setObjectPhongSize(double v)
{
    objectPhongSize_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setBumpAmount(double v)
{
    bumpAmount_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTextureRandomness(double v)
{
    textureRandomness_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setFrequency(double v)
{
    frequency_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setPhase(double v)
{
    phase_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTextureNumber(SolidTextureColorNames v)
{
    textureNumber_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setBumpNumber(SolidTextureBumpyNames v)
{
    bumpNumber_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTextureTransformation(Matrix4x4d *v)
{
    textureTransformation_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTextureTransformationInverse(Matrix4x4d *v)
{
    textureTransformationInverse_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setColor1(ColorRgba *v)
{
    color1_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setColor2(ColorRgba *v)
{
    color2_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTurbulence(double v)
{
    turbulence_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setTextureGradient(const Vector3Dd &v)
{
    textureGradient_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setColorMap(RGBAColorPalette *v)
{
    colorMap_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setImage(ControlledRGBAImageHDRUncompressed *v)
{
    image_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setBumpImage(ControlledRGBAImageHDRUncompressed *v)
{
    bumpImage_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setMaterialImage(ControlledRGBAImageHDRUncompressed *v)
{
    materialImage_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setMetallicFlag(bool v)
{
    metallicFlag_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setNumberOfWaves(int v)
{
    numberOfWaves_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setOctaves(int v)
{
    octaves_ = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setMortar(double v)
{
    mortar_ = v;
    return *this;
}

SolidTextureColorNames
PovRayMaterialBuilder::getTextureNumber() const
{
    return textureNumber_;
}

ControlledRGBAImageHDRUncompressed *
PovRayMaterialBuilder::getImage() const
{
    return image_;
}

ControlledRGBAImageHDRUncompressed *
PovRayMaterialBuilder::getBumpImage() const
{
    return bumpImage_;
}

ControlledRGBAImageHDRUncompressed *
PovRayMaterialBuilder::getMaterialImage() const
{
    return materialImage_;
}

ColorRgba *
PovRayMaterialBuilder::getColor1() const
{
    return color1_;
}

ColorRgba *
PovRayMaterialBuilder::getColor2() const
{
    return color2_;
}

int
PovRayMaterialBuilder::getOctaves() const
{
    return octaves_;
}

double
PovRayMaterialBuilder::getMortar() const
{
    return mortar_;
}

const Vector3Dd &
PovRayMaterialBuilder::getTextureGradient() const
{
    return textureGradient_;
}

java::ArrayList<PovRayMaterial *> &
PovRayMaterialBuilder::layers()
{
    return layers_;
}

java::ArrayList<PovRayMaterial *> &
PovRayMaterialBuilder::materials()
{
    return materials_;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::translate(const Vector3Dd &v)
{
    if ((textureNumber_ != SolidTextureColorNames::NO_TEXTURE &&
         textureNumber_ != SolidTextureColorNames::COLOUR_TEXTURE) ||
        bumpNumber_ != SolidTextureBumpyNames::NO_BUMPS) {

        if (!textureTransformation_) {
            textureTransformation_ = new Matrix4x4d(Matrix4x4d::identityMatrix());
            textureTransformationInverse_ = new Matrix4x4d(Matrix4x4d::identityMatrix());
        }

        Matrix4x4d deltaTransformation = Matrix4x4d().translation(
            v.x(), v.y(), v.z()).transpose();
        Matrix4x4d deltaTransformationInverse = Matrix4x4d().translation(
            0.0 - v.x(), 0.0 - v.y(), 0.0 - v.z()).transpose();

        *textureTransformation_ =
            textureTransformation_->multiply(deltaTransformation);
        *textureTransformationInverse_ = deltaTransformationInverse.multiply(
            *textureTransformationInverse_);
    }
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::rotate(Vector3Dd &v)
{
    if ((textureNumber_ != SolidTextureColorNames::NO_TEXTURE &&
         textureNumber_ != SolidTextureColorNames::COLOUR_TEXTURE) ||
        bumpNumber_ != SolidTextureBumpyNames::NO_BUMPS) {

        if (!textureTransformation_) {
            textureTransformation_ = new Matrix4x4d(Matrix4x4d::identityMatrix());
            textureTransformationInverse_ = new Matrix4x4d(Matrix4x4d::identityMatrix());
        }

        Matrix4x4d deltaTransformation;
        Matrix4x4d deltaTransformationInverse;
        deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, &v);

        *textureTransformation_ =
            textureTransformation_->multiply(deltaTransformation);
        *textureTransformationInverse_ = deltaTransformationInverse.multiply(
            *textureTransformationInverse_);
    }
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::scale(const Vector3Dd &v)
{
    if ((textureNumber_ != SolidTextureColorNames::NO_TEXTURE &&
         textureNumber_ != SolidTextureColorNames::COLOUR_TEXTURE) ||
        bumpNumber_ != SolidTextureBumpyNames::NO_BUMPS) {

        if (!textureTransformation_) {
            textureTransformation_ = new Matrix4x4d(Matrix4x4d::identityMatrix());
            textureTransformationInverse_ = new Matrix4x4d(Matrix4x4d::identityMatrix());
        }

        Matrix4x4d deltaTransformation = Matrix4x4d().scale(
            v.x(), v.y(), v.z());
        Matrix4x4d deltaTransformationInverse = Matrix4x4d().scale(
            1.0 / v.x(), 1.0 / v.y(), 1.0 / v.z());

        *textureTransformation_ =
            textureTransformation_->multiply(deltaTransformation);
        *textureTransformationInverse_ = deltaTransformationInverse.multiply(
            *textureTransformationInverse_);
    }
    return *this;
}
