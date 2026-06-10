/**
Implements texture-space transforms (translate/rotate/scale/copy) for POV-Ray
material descriptors, plus the global default texture and color-map sampling.
The Perlin noise primitives (Noise, DNoise, Turbulence, DTurbulence, cycloidal,
triangleWave) are implemented in ProceduralNoise. Color, bump, and map texture
routines are in ColorTextureFixture.cpp, BumpTextureFixture.cpp, and
MapTextureFixture.cpp respectively.
*/

#include <cstdio>
#include <cstdlib>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/statistics/SolidTextureStatistics.h"
#include "solidTexture/SolidTextureBumpyTextures.h"
#include "solidTexture/SolidTextureColorTextures.h"
#include "solidTexture/Texture.h"

static Texture *defaultTextureInstance;
static double frequencyInstance[Texture::NUMBER_OF_WAVES];
static Vector3Dd waveSourcesInstance[Texture::NUMBER_OF_WAVES];

TextureUtils* TextureUtils::inst_ = nullptr;

TextureUtils::TextureUtils(SolidTextureStatistics *stats)
    : proceduralNoise_(stats)
{
}

void
TextureUtils::initialize(SolidTextureStatistics *stats)
{
    static TextureUtils inst(stats);
    inst_ = &inst;
}

TextureUtils&
TextureUtils::instance()
{
    return *inst_;
}

ProceduralNoise&
TextureUtils::proceduralNoise()
{
    return proceduralNoise_;
}

Texture *&
TextureUtils::defaultTexture()
{
    return defaultTextureInstance;
}

double *
TextureUtils::waveFrequency()
{
    return frequencyInstance;
}

Vector3Dd *
TextureUtils::waveSources()
{
    return waveSourcesInstance;
}

double
TextureUtils::floorInline(double x)
{
    return (x >= 0.0) ? floor(x) : (0.0 - floor(0.0 - x) - 1.0);
}

double
TextureUtils::fabsInline(double x)
{
    return (x < 0.0) ? (0.0 - x) : x;
}

void
TextureUtils::computeColor(
    ColorRgba *color, RGBAColorPalette *colorMap, double value)
{
    ColorRgba *c = colorMap->evalLinear(value);
    *color = *c;
    delete c;
}

void
TextureUtils::initializeNoise()
{
    int i;
    Vector3Dd point;

    proceduralNoise_.initialize();

    for (i = 0; i < Texture::NUMBER_OF_WAVES; i++) {
        proceduralNoise_.dNoise(&point, (double)i, 0.0, 0.0);
        waveSources()[i] = point.normalizedFast();
        waveFrequency()[i] = (rand() & ProceduralNoise::RNDMASK) / ProceduralNoise::RND_DIVISOR + 0.01;
    }
}

static bool
needsTransform(const Texture *texture)
{
    return ((texture->textureNumber != (int)SolidTextureColorTextures::NO_TEXTURE) &&
               (texture->textureNumber != (int)SolidTextureColorTextures::COLOUR_TEXTURE)) ||
           (texture->bumpNumber != (int)SolidTextureBumpyTextures::NO_BUMPS);
}

static void
applyTranslationTransform(Texture *texture, Vector3Dd *vector)
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
TextureUtils::translateTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyTranslationTransform(texture, vector);
        if (texture->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
            translateTexture((Texture **)&texture->color1, vector);
            translateTexture((Texture **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Texture *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyTranslationTransform(layer, vector);
            if (layer->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
                translateTexture((Texture **)&layer->color1, vector);
                translateTexture((Texture **)&layer->color2, vector);
            }
        }
    }
}

static void
copyTextureNode(Texture *dst, const Texture *src)
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

Texture *
TextureUtils::copyTexture(Texture *texture)
{
    Texture *newHead = getTexture();
    *newHead = *texture;
    copyTextureNode(newHead, texture);

    newHead->layers.clear();
    for (long int i = 0; i < texture->layers.size(); i++) {
        Texture *src = texture->layers[i];
        Texture *copy = getTexture();
        *copy = *src;
        copyTextureNode(copy, src);
        newHead->layers.add(copy);
    }

    return newHead;
}

Texture *
TextureUtils::getTexture()
{
    Texture *newTexture;

    newTexture = new Texture;
    if (newTexture == nullptr) {
        Logger::reportMessage("Texture", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
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
    newTexture->textureNumber = (int)SolidTextureColorTextures::NO_TEXTURE;
    newTexture->textureTransformation = nullptr;
    newTexture->textureTransformationInverse = nullptr;
    newTexture->bumpNumber = (int)SolidTextureBumpyTextures::NO_BUMPS;
    newTexture->turbulence = 0.0;
    newTexture->colorMap = nullptr;
    newTexture->onceFlag = false;
    newTexture->metallicFlag = false;
    newTexture->octaves = 6;  /* dmf, for turbulence functs */
    newTexture->mortar = 0.2; /* rha, for brick texture */

    newTexture->constantFlag = true;
    newTexture->color1 = nullptr;
    newTexture->color2 = nullptr;
    *&newTexture->textureGradient = Vector3Dd(0.0, 0.0, 0.0);

    newTexture->objectIndexOfRefraction = 1.0;
    newTexture->objectTransmit = 0.0;
    newTexture->objectRefraction = 0.0;
    return (newTexture);
}

static void
applyRotationTransform(Texture *texture, Vector3Dd *vector)
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
TextureUtils::rotateTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyRotationTransform(texture, vector);
        if (texture->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
            rotateTexture((Texture **)&texture->color1, vector);
            rotateTexture((Texture **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Texture *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyRotationTransform(layer, vector);
            if (layer->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
                rotateTexture((Texture **)&layer->color1, vector);
                rotateTexture((Texture **)&layer->color2, vector);
            }
        }
    }
}

static void
applyScaleTransform(Texture *texture, Vector3Dd *vector)
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
TextureUtils::scaleTexture(Texture **texturePtr, Vector3Dd *vector)
{
    Texture *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        if (texture->constantFlag) {
            texture = copyTexture(texture);
            *texturePtr = texture;
        }
        applyScaleTransform(texture, vector);
        if (texture->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
            scaleTexture((Texture **)&texture->color1, vector);
            scaleTexture((Texture **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        Texture *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyScaleTransform(layer, vector);
            if (layer->textureNumber == (int)SolidTextureColorTextures::CHECKER_TEXTURE_TEXTURE) {
                scaleTexture((Texture **)&layer->color1, vector);
                scaleTexture((Texture **)&layer->color2, vector);
            }
        }
    }
}
