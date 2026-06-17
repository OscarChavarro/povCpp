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
    return ((texture->textureNumber != SolidTextureColorNames::NO_TEXTURE) &&
               (texture->textureNumber != SolidTextureColorNames::COLOUR_TEXTURE)) ||
           (texture->bumpNumber != SolidTextureBumpyNames::NO_BUMPS);
}
void
MaterialUtils::applyTranslationTransform(PovrayMaterial *texture, const Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->textureTransformation) {
        texture->textureTransformation =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    deltaTransformationInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}
void
MaterialUtils::translateTexture(PovrayMaterial **texturePtr, Vector3Dd *vector)
{
    PovrayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyTranslationTransform(texture, vector);
        if (texture->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
            translateTexture((PovrayMaterial **)&texture->color1, vector);
            translateTexture((PovrayMaterial **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovrayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyTranslationTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                translateTexture((PovrayMaterial **)&layer->color1, vector);
                translateTexture((PovrayMaterial **)&layer->color2, vector);
            }
        }
    }
}
void
MaterialUtils::copyTextureNode(PovrayMaterial *dst, const PovrayMaterial *src)
{
    if (dst->textureTransformation) {
        dst->textureTransformation =
            new Matrix4x4d(*src->textureTransformation);
        dst->textureTransformationInverse =
            new Matrix4x4d(*src->textureTransformationInverse);
    }
    if (dst->colorMap != nullptr) {
        RGBAColorPalette * const newMap = new RGBAColorPalette();
        for (int i = 0; i < src->colorMap->size(); i++) {
            const ColorRgba *c = src->colorMap->getColorAt(i);
            if (src->colorMap->hasPositions()) {
                newMap->addColorAt(src->colorMap->getPositionAt(i), *c);
            } else {
                newMap->addColor(*c);
            }
            delete c;
        }
        dst->colorMap = newMap;
    }
    dst->constantFlag = false;
}
PovrayMaterial *
MaterialUtils::copyTexture(PovrayMaterial *texture)
{
    PovrayMaterial * const newHead = getTexture();
    *newHead = *texture;
    copyTextureNode(newHead, texture);

    newHead->layers.clear();
    for (long int i = 0; i < texture->layers.size(); i++) {
        const PovrayMaterial *src = texture->layers[i];
        PovrayMaterial * const copy = getTexture();
        *copy = *src;
        copyTextureNode(copy, src);
        newHead->layers.add(copy);
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

    newTexture->objectReflection = 0.0;
    newTexture->objectAmbient = 0.1;
    newTexture->objectDiffuse = 0.6;
    newTexture->objectBrilliance = 1.0;
    newTexture->objectSpecular = 0.0;
    newTexture->objectRoughness = 0.05;
    newTexture->objectPhong = 0.0;
    newTexture->objectPhongSize = 40;

    newTexture->textureRandomness = 0.0;
    newTexture->bumpAmount = 0.0;
    newTexture->phase = 0.0;
    newTexture->frequency = 1.0;
    newTexture->textureNumber = SolidTextureColorNames::NO_TEXTURE;
    newTexture->textureTransformation = nullptr;
    newTexture->textureTransformationInverse = nullptr;
    newTexture->bumpNumber = SolidTextureBumpyNames::NO_BUMPS;
    newTexture->turbulence = 0.0;
    newTexture->colorMap = nullptr;
    newTexture->onceFlag = false;
    newTexture->metallicFlag = false;
    newTexture->octaves = 6;  // dmf, for turbulence functions
    newTexture->mortar = 0.2; // rha, for brick texture

    newTexture->constantFlag = true;
    newTexture->color1 = nullptr;
    newTexture->color2 = nullptr;
    *&newTexture->textureGradient = Vector3Dd(0.0, 0.0, 0.0);

    newTexture->objectIndexOfRefraction = 1.0;
    newTexture->objectTransmit = 0.0;
    newTexture->objectRefraction = 0.0;
    return (newTexture);
}
void
MaterialUtils::applyRotationTransform(PovrayMaterial *texture, Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->textureTransformation) {
        texture->textureTransformation =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, vector);
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}
void
MaterialUtils::rotateTexture(PovrayMaterial **texturePtr, Vector3Dd *vector)
{
    PovrayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyRotationTransform(texture, vector);
        if (texture->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
            rotateTexture((PovrayMaterial **)&texture->color1, vector);
            rotateTexture((PovrayMaterial **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovrayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyRotationTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                rotateTexture((PovrayMaterial **)&layer->color1, vector);
                rotateTexture((PovrayMaterial **)&layer->color2, vector);
            }
        }
    }
}
void
MaterialUtils::applyScaleTransform(PovrayMaterial *texture, const Vector3Dd *vector)
{
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;

    if (!texture->textureTransformation) {
        texture->textureTransformation =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse =
            new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    deltaTransformation = Matrix4x4d().scale(vector->x(), vector->y(), vector->z());
    deltaTransformationInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}
void
MaterialUtils::scaleTexture(PovrayMaterial **texturePtr, Vector3Dd *vector)
{
    PovrayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyScaleTransform(texture, vector);
        if (texture->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
            scaleTexture((PovrayMaterial **)&texture->color1, vector);
            scaleTexture((PovrayMaterial **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovrayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyScaleTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                scaleTexture((PovrayMaterial **)&layer->color1, vector);
                scaleTexture((PovrayMaterial **)&layer->color2, vector);
            }
        }
    }
}
