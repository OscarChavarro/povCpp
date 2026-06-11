/**
Material utilities: global default texture management.
*/

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"
#include "environment/material/MaterialUtils.h"
#include "environment/material/Material.h"

static Material *defaultTextureInstance;

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

Material *
MaterialUtils::defaultTexture()
{
    return defaultTextureInstance;
}

void
MaterialUtils::setDefaultTexture(Material *texture)
{
    defaultTextureInstance = texture;
}

bool
MaterialUtils::needsTransform(const Material *texture)
{
    return ((texture->textureNumber != SolidTextureColorNames::NO_TEXTURE) &&
               (texture->textureNumber != SolidTextureColorNames::COLOUR_TEXTURE)) ||
           (texture->bumpNumber != SolidTextureBumpyNames::NO_BUMPS);
}
void
MaterialUtils::applyTranslationTransform(Material *texture, Vector3Dd *vector)
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
MaterialUtils::translateTexture(Material **texturePtr, Vector3Dd *vector)
{
    Material *texture = *texturePtr;
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
            translateTexture((Material **)&texture->color1, vector);
            translateTexture((Material **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Material *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyTranslationTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                translateTexture((Material **)&layer->color1, vector);
                translateTexture((Material **)&layer->color2, vector);
            }
        }
    }
}
void
MaterialUtils::copyTextureNode(Material *dst, const Material *src)
{
    if (dst->textureTransformation) {
        dst->textureTransformation =
            new Matrix4x4d(*src->textureTransformation);
        dst->textureTransformationInverse =
            new Matrix4x4d(*src->textureTransformationInverse);
    }
    if (dst->colorMap != nullptr) {
        RGBAColorPalette *newMap = new RGBAColorPalette();
        for (int i = 0; i < src->colorMap->size(); i++) {
            ColorRgba *c = src->colorMap->getColorAt(i);
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
Material *
MaterialUtils::copyTexture(Material *texture)
{
    Material *newHead = getTexture();
    *newHead = *texture;
    copyTextureNode(newHead, texture);

    newHead->layers.clear();
    for (long int i = 0; i < texture->layers.size(); i++) {
        Material *src = texture->layers[i];
        Material *copy = getTexture();
        *copy = *src;
        copyTextureNode(copy, src);
        newHead->layers.add(copy);
    }

    return newHead;
}
Material *
MaterialUtils::getTexture()
{
    Material *newTexture;

    newTexture = new Material;
    if (newTexture == nullptr) {
        Logger::reportMessage("Material", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
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
MaterialUtils::applyRotationTransform(Material *texture, Vector3Dd *vector)
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
MaterialUtils::rotateTexture(Material **texturePtr, Vector3Dd *vector)
{
    Material *texture = *texturePtr;
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
            rotateTexture((Material **)&texture->color1, vector);
            rotateTexture((Material **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Material *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyRotationTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                rotateTexture((Material **)&layer->color1, vector);
                rotateTexture((Material **)&layer->color2, vector);
            }
        }
    }
}
void
MaterialUtils::applyScaleTransform(Material *texture, Vector3Dd *vector)
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
MaterialUtils::scaleTexture(Material **texturePtr, Vector3Dd *vector)
{
    Material *texture = *texturePtr;
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
            scaleTexture((Material **)&texture->color1, vector);
            scaleTexture((Material **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Material *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyScaleTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                scaleTexture((Material **)&layer->color1, vector);
                scaleTexture((Material **)&layer->color2, vector);
            }
        }
    }
}
