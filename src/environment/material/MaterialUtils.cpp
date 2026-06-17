/**
PovrayMaterial utilities: global default texture management.
*/

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"

#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"
#include "environment/material/PovrayMaterial.h"
#include "environment/material/MaterialUtils.h"

#include "java/util/ArrayList.txx"

static PovrayMaterial *defaultTextureInstance;

MaterialUtils* MaterialUtils::materialInstance = nullptr;

MaterialUtils::MaterialUtils()
{
}

void
MaterialUtils::initialize()
{
    static MaterialUtils inst;
    materialInstance = &inst;
}

MaterialUtils&
MaterialUtils::instance()
{
    return *materialInstance;
}

PovrayMaterial *
MaterialUtils::defaultTexture()
{
    return defaultTextureInstance;
}

void
MaterialUtils::setDefaultTexture(PovrayMaterial *texture)
{
    defaultTextureInstance = texture;
}

bool
MaterialUtils::needsTransform(const PovrayMaterial *texture)
{
    return ((texture->getTextureNumber() != SolidTextureColorNames::NO_TEXTURE) &&
               (texture->getTextureNumber() != SolidTextureColorNames::COLOUR_TEXTURE)) ||
           (texture->getBumpNumber() != SolidTextureBumpyNames::NO_BUMPS);
}
void
MaterialUtils::applyTranslationTransform(PovrayMaterial *texture, const Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->getTextureTransformation()) {
        texture->setTextureTransformation(
            new Matrix4x4d(Matrix4x4d::identityMatrix()));
        texture->setTextureTransformationInverse(
            new Matrix4x4d(Matrix4x4d::identityMatrix()));
    }
    deltaTransformation = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    deltaTransformationInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *texture->getTextureTransformation() =
        texture->getTextureTransformation()->multiply(deltaTransformation);
    *texture->getTextureTransformationInverse() = deltaTransformationInverse.multiply(
        *texture->getTextureTransformationInverse());
}
void
MaterialUtils::translateTexture(PovrayMaterial **texturePtr, Vector3Dd *vector)
{
    PovrayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->isConstant()) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyTranslationTransform(texture, vector);
        if (texture->getTextureNumber() == (int)CHECKER_TEXTURE_TEXTURE) {
            translateTexture((PovrayMaterial **)&texture->getColor1(), vector);
            translateTexture((PovrayMaterial **)&texture->getColor2(), vector);
        }
    }

    for (long int i = 0; i < texture->getLayers().size(); i++) {
        PovrayMaterial *layer = texture->getLayers()[i];
        if (needsTransform(layer)) {
            if (layer->isConstant()) {
                layer = copyTexture(layer);
                texture->getLayers()[i] = layer;
            }
            applyTranslationTransform(layer, vector);
            if (layer->getTextureNumber() == (int)CHECKER_TEXTURE_TEXTURE) {
                translateTexture((PovrayMaterial **)&layer->getColor1(), vector);
                translateTexture((PovrayMaterial **)&layer->getColor2(), vector);
            }
        }
    }
}
void
MaterialUtils::copyTextureNode(PovrayMaterial *dst, const PovrayMaterial *src)
{
    if (dst->getTextureTransformation()) {
        dst->setTextureTransformation(
            new Matrix4x4d(*src->getTextureTransformation()));
        dst->setTextureTransformationInverse(
            new Matrix4x4d(*src->getTextureTransformationInverse()));
    }
    if (dst->getColorMap() != nullptr) {
        RGBAColorPalette * const newMap = new RGBAColorPalette();
        for (int i = 0; i < src->getColorMap()->size(); i++) {
            const ColorRgba *c = src->getColorMap()->getColorAt(i);
            if (src->getColorMap()->hasPositions()) {
                newMap->addColorAt(src->getColorMap()->getPositionAt(i), *c);
            } else {
                newMap->addColor(*c);
            }
            delete c;
        }
        dst->setColorMap(newMap);
    }
    dst->setConstant(false);
}
PovrayMaterial *
MaterialUtils::copyTexture(PovrayMaterial *texture)
{
    PovrayMaterial * const newHead = getTexture();
    *newHead = *texture;
    copyTextureNode(newHead, texture);

    newHead->getLayers().clear();
    for (long int i = 0; i < texture->getLayers().size(); i++) {
        const PovrayMaterial *src = texture->getLayers()[i];
        PovrayMaterial * const copy = getTexture();
        *copy = *src;
        copyTextureNode(copy, src);
        newHead->getLayers().add(copy);
    }

    return newHead;
}
PovrayMaterial *
MaterialUtils::getTexture()
{
    PovrayMaterial *newTexture;

    newTexture = new PovrayMaterial;
    if (newTexture == nullptr) {
        Logger::reportMessage("PovrayMaterial", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }

    newTexture->setObjectReflection(0.0);
    newTexture->setObjectAmbient(0.1);
    newTexture->setObjectDiffuse(0.6);
    newTexture->setObjectBrilliance(1.0);
    newTexture->setObjectSpecular(0.0);
    newTexture->setObjectRoughness(0.05);
    newTexture->setObjectPhong(0.0);
    newTexture->setObjectPhongSize(40);

    newTexture->setTextureRandomness(0.0);
    newTexture->setBumpAmount(0.0);
    newTexture->setPhase(0.0);
    newTexture->setFrequency(1.0);
    newTexture->setTextureNumber(SolidTextureColorNames::NO_TEXTURE);
    newTexture->setTextureTransformation(nullptr);
    newTexture->setTextureTransformationInverse(nullptr);
    newTexture->setBumpNumber(SolidTextureBumpyNames::NO_BUMPS);
    newTexture->setTurbulence(0.0);
    newTexture->setColorMap(nullptr);
    newTexture->setOnceFlag(false);
    newTexture->setMetallicFlag(false);
    newTexture->setOctaves(6);  // dmf, for turbulence functions
    newTexture->setMortar(0.2); // rha, for brick texture

    newTexture->setConstant(true);
    newTexture->setColor1(nullptr);
    newTexture->setColor2(nullptr);
    newTexture->setTextureGradient(Vector3Dd(0.0, 0.0, 0.0));

    newTexture->setObjectIndexOfRefraction(1.0);
    newTexture->setObjectTransmit(0.0);
    newTexture->setObjectRefraction(0.0);
    return (newTexture);
}
void
MaterialUtils::applyRotationTransform(PovrayMaterial *texture, Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->getTextureTransformation()) {
        texture->setTextureTransformation(
            new Matrix4x4d(Matrix4x4d::identityMatrix()));
        texture->setTextureTransformationInverse(
            new Matrix4x4d(Matrix4x4d::identityMatrix()));
    }
    deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, vector);
    *texture->getTextureTransformation() =
        texture->getTextureTransformation()->multiply(deltaTransformation);
    *texture->getTextureTransformationInverse() = deltaTransformationInverse.multiply(
        *texture->getTextureTransformationInverse());
}
void
MaterialUtils::rotateTexture(PovrayMaterial **texturePtr, Vector3Dd *vector)
{
    PovrayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->isConstant()) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyRotationTransform(texture, vector);
        if (texture->getTextureNumber() == (int)CHECKER_TEXTURE_TEXTURE) {
            rotateTexture((PovrayMaterial **)&texture->getColor1(), vector);
            rotateTexture((PovrayMaterial **)&texture->getColor2(), vector);
        }
    }

    for (long int i = 0; i < texture->getLayers().size(); i++) {
        PovrayMaterial *layer = texture->getLayers()[i];
        if (needsTransform(layer)) {
            if (layer->isConstant()) {
                layer = copyTexture(layer);
                texture->getLayers()[i] = layer;
            }
            applyRotationTransform(layer, vector);
            if (layer->getTextureNumber() == (int)CHECKER_TEXTURE_TEXTURE) {
                rotateTexture((PovrayMaterial **)&layer->getColor1(), vector);
                rotateTexture((PovrayMaterial **)&layer->getColor2(), vector);
            }
        }
    }
}
void
MaterialUtils::applyScaleTransform(PovrayMaterial *texture, const Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->getTextureTransformation()) {
        texture->setTextureTransformation(
            new Matrix4x4d(Matrix4x4d::identityMatrix()));
        texture->setTextureTransformationInverse(
            new Matrix4x4d(Matrix4x4d::identityMatrix()));
    }
    deltaTransformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    deltaTransformationInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *texture->getTextureTransformation() =
        texture->getTextureTransformation()->multiply(deltaTransformation);
    *texture->getTextureTransformationInverse() = deltaTransformationInverse.multiply(
        *texture->getTextureTransformationInverse());
}
void
MaterialUtils::scaleTexture(PovrayMaterial **texturePtr, Vector3Dd *vector)
{
    PovrayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->isConstant()) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyScaleTransform(texture, vector);
        if (texture->getTextureNumber() == (int)CHECKER_TEXTURE_TEXTURE) {
            scaleTexture((PovrayMaterial **)&texture->getColor1(), vector);
            scaleTexture((PovrayMaterial **)&texture->getColor2(), vector);
        }
    }

    for (long int i = 0; i < texture->getLayers().size(); i++) {
        PovrayMaterial *layer = texture->getLayers()[i];
        if (needsTransform(layer)) {
            if (layer->isConstant()) {
                layer = copyTexture(layer);
                texture->getLayers()[i] = layer;
            }
            applyScaleTransform(layer, vector);
            if (layer->getTextureNumber() == (int)CHECKER_TEXTURE_TEXTURE) {
                scaleTexture((PovrayMaterial **)&layer->getColor1(), vector);
                scaleTexture((PovrayMaterial **)&layer->getColor2(), vector);
            }
        }
    }
}
