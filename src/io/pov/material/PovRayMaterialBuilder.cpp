#include "io/pov/material/PovRayMaterialBuilder.h"
#include "environment/material/normal/BumpMapNormal.h"
#include "environment/material/normal/BumpsNormal.h"
#include "environment/material/normal/DentsNormal.h"
#include "environment/material/normal/RipplesNormal.h"
#include "environment/material/normal/UnsupportedBumpNormal.h"
#include "environment/material/normal/WavesNormal.h"
#include "environment/material/normal/WrinklesNormal.h"
#include "environment/material/pigment/AgatePigment.h"
#include "environment/material/pigment/BozoPigment.h"
#include "environment/material/pigment/BrickPigment.h"
#include "environment/material/pigment/CheckerColorPigment.h"
#include "environment/material/pigment/CheckerTexturePigment.h"
#include "environment/material/pigment/ColourPigment.h"
#include "environment/material/pigment/GradientPigment.h"
#include "environment/material/pigment/GranitePigment.h"
#include "environment/material/pigment/ImageMapPigment.h"
#include "environment/material/pigment/LeopardPigment.h"
#include "environment/material/pigment/MarblePigment.h"
#include "environment/material/pigment/MaterialMapPigment.h"
#include "environment/material/pigment/OnionPigment.h"
#include "environment/material/pigment/SpottedPigment.h"
#include "environment/material/pigment/WoodPigment.h"
#include "java/util/ArrayList.txx"

PovRayMaterialBuilder::PovRayMaterialBuilder() :
    bumpAmount(0.0),
    bumpImage(nullptr),
    bumpNumber(SolidTextureBumpyNames::NO_BUMPS),
    checkerColor1(nullptr),
    checkerColor2(nullptr),
    checkerTexture1(nullptr),
    checkerTexture2(nullptr),
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
    bumpAmount(base->getPendingBumpAmount()),
    bumpImage(base->getPendingBumpImage()),
    bumpNumber(SolidTextureBumpyNames::NO_BUMPS),
    checkerColor1(nullptr),
    checkerColor2(nullptr),
    checkerTexture1(nullptr),
    checkerTexture2(nullptr),
    colorMap(SolidTexturePigment::cloneColorMap(base->getPendingColorMap())),
    frequency(base->getPendingFrequency()),
    image(nullptr),
    materialImage(base->getMaterialMapImage()),
    metallicFlag(base->isMetallic()),
    mortar(base->getPendingMortar()),
    numberOfWaves(base->getPendingNumberOfWaves()),
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
    octaves(base->getPendingOctaves()),
    phase(base->getPendingPhase()),
    textureGradient(0.0, 0.0, 0.0),
    textureNumber(SolidTextureColorNames::NO_TEXTURE),
    textureRandomness(base->getTextureRandomness()),
    textureTransformation(nullptr),
    textureTransformationInverse(nullptr),
    turbulence(base->getPendingTurbulence())
{
    if (base->getTextureTransformation() != nullptr) {
        textureTransformation = new Matrix4x4d(*base->getTextureTransformation());
        textureTransformationInverse = new Matrix4x4d(*base->getTextureTransformationInverse());
    }

    // Reconstructs the pattern-defining fields from the concrete pigment/normal so that
    // TextureParser's incremental single-attribute edits (rebuild via build()) keep working.
    // turbulence/octaves/colorMap/mortar/bumpAmount/frequency/phase/numberOfWaves/bumpImage
    // come from PovRayMaterial's pending* fields above instead, since those attributes can be
    // set by TextureParser *before* the pattern/bump keyword that actually creates a pigment/
    // normal object (e.g. "turbulence 0.8 bozo"), and would otherwise be lost on that build()
    // round trip while pigment/normal is still nullptr.
    SolidTexturePigment * const pigment = base->getPigment();
    if (const ColourPigment *p = dynamic_cast<const ColourPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::COLOUR_TEXTURE;
        checkerColor1 = new ColorRgba(*p->getColor1());
    } else if (dynamic_cast<const BozoPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::BOZO_TEXTURE;
    } else if (dynamic_cast<const MarblePigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::MARBLE_TEXTURE;
    } else if (dynamic_cast<const WoodPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::WOOD_TEXTURE;
    } else if (const CheckerColorPigment *p = dynamic_cast<const CheckerColorPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::CHECKER_TEXTURE;
        checkerColor1 = new ColorRgba(*p->getColor1());
        checkerColor2 = new ColorRgba(*p->getColor2());
    } else if (const CheckerTexturePigment *p = dynamic_cast<const CheckerTexturePigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::CHECKER_TEXTURE_TEXTURE;
        checkerTexture1 = (p->getTexture1() != nullptr) ?
            static_cast<PovRayMaterial *>(p->getTexture1()->copySlot()) : nullptr;
        checkerTexture2 = (p->getTexture2() != nullptr) ?
            static_cast<PovRayMaterial *>(p->getTexture2()->copySlot()) : nullptr;
    } else if (dynamic_cast<const SpottedPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::SPOTTED_TEXTURE;
    } else if (dynamic_cast<const AgatePigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::AGATE_TEXTURE;
    } else if (dynamic_cast<const GranitePigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::GRANITE_TEXTURE;
    } else if (const GradientPigment *p = dynamic_cast<const GradientPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::GRADIENT_TEXTURE;
        textureGradient = p->getTextureGradient();
    } else if (const ImageMapPigment *p = dynamic_cast<const ImageMapPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::IMAGE_MAP_TEXTURE;
        image = const_cast<ControlledRGBAImageHDRUncompressed *>(p->getImage());
    } else if (dynamic_cast<const OnionPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::ONION_TEXTURE;
    } else if (dynamic_cast<const LeopardPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::LEOPARD_TEXTURE;
    } else if (const BrickPigment *p = dynamic_cast<const BrickPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::BRICK_TEXTURE;
        checkerColor1 = new ColorRgba(*p->getColor1());
        checkerColor2 = new ColorRgba(*p->getColor2());
    } else if (dynamic_cast<const MaterialMapPigment *>(pigment)) {
        textureNumber = SolidTextureColorNames::MATERIAL_MAP_TEXTURE;
    } else {
        textureNumber = SolidTextureColorNames::NO_TEXTURE;
    }

    SolidTextureNormal * const normal = base->getNormal();
    if (dynamic_cast<const WavesNormal *>(normal)) {
        bumpNumber = SolidTextureBumpyNames::WAVES;
    } else if (dynamic_cast<const RipplesNormal *>(normal)) {
        bumpNumber = SolidTextureBumpyNames::RIPPLES;
    } else if (dynamic_cast<const WrinklesNormal *>(normal)) {
        bumpNumber = SolidTextureBumpyNames::WRINKLES;
    } else if (dynamic_cast<const BumpsNormal *>(normal)) {
        bumpNumber = SolidTextureBumpyNames::BUMPS;
    } else if (dynamic_cast<const DentsNormal *>(normal)) {
        bumpNumber = SolidTextureBumpyNames::DENTS;
    } else if (dynamic_cast<const BumpMapNormal *>(normal)) {
        bumpNumber = SolidTextureBumpyNames::BUMP_MAP;
    } else if (dynamic_cast<const UnsupportedBumpNormal *>(normal)) {
        // BUMPY1/2/3 are indistinguishable from each other once built (none of them carry
        // any state and none has any visual effect - see UnsupportedBumpNormal); BUMPY1 is
        // an arbitrary, behaviorally-equivalent pick to satisfy the needsTransform() gate.
        bumpNumber = SolidTextureBumpyNames::BUMPY1;
    } else {
        bumpNumber = SolidTextureBumpyNames::NO_BUMPS;
    }

    for (long int i = 0; i < base->getLayers().size(); i++) {
        layers.add(new PovRayMaterial(*base->getLayers()[i]));
    }
    for (long int i = 0; i < base->getMaterialMapVariants().size(); i++) {
        materials.add(new PovRayMaterial(*base->getMaterialMapVariants()[i]));
    }
}

SolidTexturePigment *
PovRayMaterialBuilder::buildPigment() const
{
    switch (textureNumber) {
    case SolidTextureColorNames::COLOUR_TEXTURE:
        return new ColourPigment(checkerColor1 != nullptr ? new ColorRgba(*checkerColor1) : nullptr);
    case SolidTextureColorNames::BOZO_TEXTURE:
        return new BozoPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
    case SolidTextureColorNames::MARBLE_TEXTURE:
        return new MarblePigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
    case SolidTextureColorNames::WOOD_TEXTURE:
        return new WoodPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
    case SolidTextureColorNames::CHECKER_TEXTURE:
        return new CheckerColorPigment(
            checkerColor1 != nullptr ? new ColorRgba(*checkerColor1) : nullptr,
            checkerColor2 != nullptr ? new ColorRgba(*checkerColor2) : nullptr);
    case SolidTextureColorNames::CHECKER_TEXTURE_TEXTURE:
        return new CheckerTexturePigment(checkerTexture1, checkerTexture2);
    case SolidTextureColorNames::SPOTTED_TEXTURE:
        return new SpottedPigment(SolidTexturePigment::cloneColorMap(colorMap));
    case SolidTextureColorNames::AGATE_TEXTURE:
        return new AgatePigment(octaves, SolidTexturePigment::cloneColorMap(colorMap));
    case SolidTextureColorNames::GRANITE_TEXTURE:
        return new GranitePigment(SolidTexturePigment::cloneColorMap(colorMap));
    case SolidTextureColorNames::GRADIENT_TEXTURE:
        return new GradientPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap), textureGradient);
    case SolidTextureColorNames::IMAGE_MAP_TEXTURE:
        return new ImageMapPigment(image);
    case SolidTextureColorNames::ONION_TEXTURE:
        return new OnionPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
    case SolidTextureColorNames::LEOPARD_TEXTURE:
        return new LeopardPigment(turbulence, octaves, SolidTexturePigment::cloneColorMap(colorMap));
    case SolidTextureColorNames::BRICK_TEXTURE:
        return new BrickPigment(
            checkerColor1 != nullptr ? new ColorRgba(*checkerColor1) : nullptr,
            checkerColor2 != nullptr ? new ColorRgba(*checkerColor2) : nullptr,
            mortar);
    case SolidTextureColorNames::MATERIAL_MAP_TEXTURE:
        return new MaterialMapPigment();
    case SolidTextureColorNames::NO_TEXTURE:
    default:
        return nullptr;
    }
}

SolidTextureNormal *
PovRayMaterialBuilder::buildNormal() const
{
    switch (bumpNumber) {
    case SolidTextureBumpyNames::WAVES:
        return new WavesNormal(bumpAmount, frequency, phase, numberOfWaves);
    case SolidTextureBumpyNames::RIPPLES:
        return new RipplesNormal(bumpAmount, frequency, phase, numberOfWaves);
    case SolidTextureBumpyNames::WRINKLES:
        return new WrinklesNormal(bumpAmount);
    case SolidTextureBumpyNames::BUMPS:
        return new BumpsNormal(bumpAmount);
    case SolidTextureBumpyNames::DENTS:
        return new DentsNormal(bumpAmount);
    case SolidTextureBumpyNames::BUMPY1:
    case SolidTextureBumpyNames::BUMPY2:
    case SolidTextureBumpyNames::BUMPY3:
        return new UnsupportedBumpNormal();
    case SolidTextureBumpyNames::BUMP_MAP:
        return new BumpMapNormal(bumpAmount, bumpImage);
    case SolidTextureBumpyNames::NO_BUMPS:
    default:
        return nullptr;
    }
}

PovRayMaterial *
PovRayMaterialBuilder::build() const
{
    SolidTexturePigment * const pigment = buildPigment();
    SolidTextureNormal * const normal = buildNormal();
    // buildPigment() now always clones colorMap/checkerColor1/checkerColor2 for
    // the pigment types that use them (never aliases this builder's own copies
    // - see the cloneColorMap/new ColorRgba calls above), and the
    // PovRayMaterial below gets its own independent colorMap clone too, so
    // none of these three are ever transferred anywhere and are always safe to
    // free here once buildPigment() has made its own clones.
    // checkerTexture1/checkerTexture2 are deliberately NOT freed here: unlike
    // these, CheckerTexturePigment takes them as-is (no clone), and their only
    // setters (setCheckerTexture1/2) absorb the *previous* value as a new
    // layer via PovRayMaterialUtils::prependTextureLayers rather than
    // discarding it - freeing them here would double-free an object still
    // reachable through whatever they were just reassigned to.
    RGBAColorPalette * const pendingColorMapClone =
        SolidTexturePigment::cloneColorMap(colorMap);
    delete colorMap;
    delete checkerColor1;
    delete checkerColor2;
    return new PovRayMaterial(objectReflection, objectAmbient, objectDiffuse,
        objectBrilliance, objectIndexOfRefraction, objectRefraction,
        objectTransmit, objectSpecular, objectRoughness, objectPhong,
        objectPhongSize, textureRandomness,
        textureTransformation, textureTransformationInverse,
        pigment, normal,
        materialImage, metallicFlag, layers, materials,
        turbulence, octaves, pendingColorMapClone,
        mortar, bumpAmount, frequency, phase, numberOfWaves, bumpImage);
}

ColorRgba *
PovRayMaterialBuilder::getCheckerColor1() const
{
    return checkerColor1;
}

ColorRgba *
PovRayMaterialBuilder::getCheckerColor2() const
{
    return checkerColor2;
}

PovRayMaterial *
PovRayMaterialBuilder::getCheckerTexture1() const
{
    return checkerTexture1;
}

PovRayMaterial *
PovRayMaterialBuilder::getCheckerTexture2() const
{
    return checkerTexture2;
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
PovRayMaterialBuilder::setCheckerColor1(ColorRgba *v)
{
    // checkerColor1 may already hold a clone the "rebuild from base" constructor
    // made (see the ColourPigment/CheckerColorPigment/BrickPigment branches
    // above) that this attribute token's own fresh allocation is about to
    // replace. Unlike setCheckerTexture1/2 below, every call site here passes a
    // brand-new ColorRgba - never one derived from the current value via
    // PovRayMaterialUtils::prependTextureLayers - so the old value is never
    // aliased elsewhere and is safe to free.
    if (checkerColor1 != v) {
        delete checkerColor1;
    }
    checkerColor1 = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setCheckerColor2(ColorRgba *v)
{
    if (checkerColor2 != v) {
        delete checkerColor2;
    }
    checkerColor2 = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setCheckerTexture1(PovRayMaterial *v)
{
    // Unlike setCheckerColor1/2 above, every call site here passes a value
    // derived from the *current* checkerTexture1 via
    // PovRayMaterialUtils::prependTextureLayers(), which absorbs the old
    // PovRayMaterial as a layer of the new one rather than discarding it - so
    // the old value must not be freed here, it is still reachable through `v`.
    checkerTexture1 = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setCheckerTexture2(PovRayMaterial *v)
{
    checkerTexture2 = v;
    return *this;
}

PovRayMaterialBuilder &
PovRayMaterialBuilder::setColorMap(RGBAColorPalette *v)
{
    // colorMap always starts out as a clone of base->getPendingColorMap() (set
    // in the constructor, for every pattern type) before any color_map{} token
    // is parsed for this generation; the sole call site passes a freshly
    // parsed RGBAColorPalette, never one derived from the current value, so
    // the old clone is never aliased elsewhere and is safe to free here.
    if (colorMap != v) {
        delete colorMap;
    }
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
