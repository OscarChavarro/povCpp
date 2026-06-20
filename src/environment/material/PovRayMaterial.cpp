#include "environment/material/PovRayMaterial.h"
#include "environment/material/SolidTextureBumpyNames.h"
#include "environment/material/SolidTextureColorNames.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/media/RGBAColorPalette.h"

PovRayMaterial::PovRayMaterial(
    double objReflection, double objAmbient, double objDiffuse,
    double objBrilliance, double objIndexOfRefraction, double objRefraction,
    double objTransmit, double objSpecular, double objRoughness,
    double objPhong, double objPhongSize, double bump,
    double texRandomness, double freq, double ph,
    SolidTextureColorNames texNum, SolidTextureBumpyNames bumpNum,
    Matrix4x4d *texTransform, Matrix4x4d *texTransformInv,
    ColorRgba *col1, ColorRgba *col2, double turb,
    const Vector3Dd &texGrad, RGBAColorPalette *colorPal,
    ControlledRGBAImageHDRUncompressed *img,
    ControlledRGBAImageHDRUncompressed *bumpImg,
    ControlledRGBAImageHDRUncompressed *materialImg, bool metallic,
    bool once, bool constant, int numWaves, int oct, double mort,
    java::ArrayList<PovRayMaterial *> layersList,
    java::ArrayList<PovRayMaterial *> materialsList) :
    layers(layersList),
    materials(materialsList),
    objectReflection(objReflection),
    objectAmbient(objAmbient),
    objectDiffuse(objDiffuse),
    objectBrilliance(objBrilliance),
    objectIndexOfRefraction(objIndexOfRefraction),
    objectRefraction(objRefraction),
    objectTransmit(objTransmit),
    objectSpecular(objSpecular),
    objectRoughness(objRoughness),
    objectPhong(objPhong),
    objectPhongSize(objPhongSize),
    bumpAmount(bump),
    textureRandomness(texRandomness),
    frequency(freq),
    phase(ph),
    textureNumber(texNum),
    bumpNumber(bumpNum),
    textureTransformation(texTransform),
    textureTransformationInverse(texTransformInv),
    color1(col1),
    color2(col2),
    turbulence(turb),
    textureGradient(texGrad),
    colorMap(colorPal),
    image(img),
    bumpImage(bumpImg),
    materialImage(materialImg),
    metallicFlag(metallic),
    onceFlag(once),
    constantFlag(constant),
    numberOfWaves(numWaves),
    octaves(oct),
    mortar(mort)
{
}

PovRayMaterial *
PovRayMaterial::copy()
{
    return copyTexture(this);
}

Material *
PovRayMaterial::translate(Vector3Dd *vector)
{
    // translateTexture copy-on-writes a constant material into a throwaway copy and
    // leaves `this` untouched, so the body keeps `this` unchanged (the §1.4 no-op quirk).
    // For a non-constant material the transform is applied in place and `self` stays `this`.
    PovRayMaterial *self = this;
    translateTexture(&self, vector);
    return this;
}

Material *
PovRayMaterial::rotate(Vector3Dd *vector)
{
    PovRayMaterial *self = this;
    rotateTexture(&self, vector);
    return this;
}

Material *
PovRayMaterial::scale(Vector3Dd *vector)
{
    PovRayMaterial *self = this;
    scaleTexture(&self, vector);
    return this;
}

Material *
PovRayMaterial::prependMaterialLayers(Material *existingMaterial)
{
    PovRayMaterial *existingPovRay = static_cast<PovRayMaterial *>(existingMaterial);
    if (existingPovRay != nullptr && existingPovRay != this) {
        layers.add(existingPovRay);
        for (long int i = 0; i < existingPovRay->layers.size(); i++) {
            layers.add(existingPovRay->layers[i]);
        }
        existingPovRay->layers.clear();
    }
    return this;
}

// ---------------------------------------------------------------------------
// Texture-space copy/transform engine (own member functions; the only writers
// of the private state besides the constructors). Parse-time only.
// ---------------------------------------------------------------------------

bool
PovRayMaterial::needsTransform(const PovRayMaterial *texture)
{
    return ((texture->textureNumber != SolidTextureColorNames::NO_TEXTURE) &&
               (texture->textureNumber != SolidTextureColorNames::COLOUR_TEXTURE)) ||
           (texture->bumpNumber != SolidTextureBumpyNames::NO_BUMPS);
}

PovRayMaterial *
PovRayMaterial::copyTextureNode(const PovRayMaterial *src)
{
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    if (src->textureTransformation != nullptr) {
        transformation = new Matrix4x4d(*src->textureTransformation);
        transformationInverse = new Matrix4x4d(*src->textureTransformationInverse);
    }
    RGBAColorPalette *newColorMap = nullptr;
    if (src->colorMap != nullptr) {
        newColorMap = new RGBAColorPalette();
        for (int i = 0; i < src->colorMap->size(); i++) {
            const ColorRgba *c = src->colorMap->getColorAt(i);
            if (src->colorMap->hasPositions()) {
                newColorMap->addColorAt(src->colorMap->getPositionAt(i), *c);
            } else {
                newColorMap->addColor(*c);
            }
            delete c;
        }
    }
    return new PovRayMaterial(
        src->objectReflection, src->objectAmbient, src->objectDiffuse,
        src->objectBrilliance, src->objectIndexOfRefraction, src->objectRefraction,
        src->objectTransmit, src->objectSpecular, src->objectRoughness,
        src->objectPhong, src->objectPhongSize, src->bumpAmount,
        src->textureRandomness, src->frequency, src->phase,
        src->textureNumber, src->bumpNumber, transformation, transformationInverse,
        src->color1, src->color2, src->turbulence,
        src->textureGradient, newColorMap, src->image, src->bumpImage, src->materialImage,
        src->metallicFlag, src->onceFlag, false, src->numberOfWaves, src->octaves, src->mortar,
        src->layers, src->materials);
}

PovRayMaterial *
PovRayMaterial::copyTexture(const PovRayMaterial *texture)
{
    PovRayMaterial * const newHead = copyTextureNode(texture);

    newHead->layers.clear();
    for (long int i = 0; i < texture->layers.size(); i++) {
        newHead->layers.add(copyTextureNode(texture->layers[i]));
    }

    return newHead;
}

void
PovRayMaterial::prependTextureLayers(
    PovRayMaterial *newHead, PovRayMaterial *&existingHead)
{
    if (existingHead != nullptr) {
        newHead->layers.add(existingHead);
        for (long int i = 0; i < existingHead->layers.size(); i++) {
            newHead->layers.add(existingHead->layers[i]);
        }
        existingHead->layers.clear();
    }
    existingHead = newHead;
}

void
PovRayMaterial::applyTranslationTransform(
    PovRayMaterial *texture, const Vector3Dd *vector)
{
    if (!texture->textureTransformation) {
        texture->textureTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    Matrix4x4d deltaTransformation = Matrix4x4d().translation(
        vector->x(), vector->y(), vector->z()).transpose();
    Matrix4x4d deltaTransformationInverse = Matrix4x4d().translation(
        0.0 - vector->x(), 0.0 - vector->y(), 0.0 - vector->z()).transpose();
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}

void
PovRayMaterial::applyRotationTransform(
    PovRayMaterial *texture, Vector3Dd *vector)
{
    if (!texture->textureTransformation) {
        texture->textureTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    Matrix4x4d deltaTransformation;
    Matrix4x4d deltaTransformationInverse;
    deltaTransformation.axisRotationRodrigues(&deltaTransformationInverse, vector);
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}

void
PovRayMaterial::applyScaleTransform(
    PovRayMaterial *texture, const Vector3Dd *vector)
{
    if (!texture->textureTransformation) {
        texture->textureTransformation = new Matrix4x4d(Matrix4x4d::identityMatrix());
        texture->textureTransformationInverse = new Matrix4x4d(Matrix4x4d::identityMatrix());
    }
    Matrix4x4d deltaTransformation = Matrix4x4d().scale(
        vector->x(), vector->y(), vector->z());
    Matrix4x4d deltaTransformationInverse = Matrix4x4d().scale(
        1.0 / vector->x(), 1.0 / vector->y(), 1.0 / vector->z());
    *texture->textureTransformation =
        texture->textureTransformation->multiply(deltaTransformation);
    *texture->textureTransformationInverse = deltaTransformationInverse.multiply(
        *texture->textureTransformationInverse);
}

void
PovRayMaterial::translateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial *texture = *texturePtr;
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
            translateTexture((PovRayMaterial **)&texture->color1, vector);
            translateTexture((PovRayMaterial **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovRayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyTranslationTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                translateTexture((PovRayMaterial **)&layer->color1, vector);
                translateTexture((PovRayMaterial **)&layer->color2, vector);
            }
        }
    }
}

void
PovRayMaterial::rotateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial *texture = *texturePtr;
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
            rotateTexture((PovRayMaterial **)&texture->color1, vector);
            rotateTexture((PovRayMaterial **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovRayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyRotationTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                rotateTexture((PovRayMaterial **)&layer->color1, vector);
                rotateTexture((PovRayMaterial **)&layer->color2, vector);
            }
        }
    }
}

void
PovRayMaterial::scaleTexture(PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial *texture = *texturePtr;
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
            scaleTexture((PovRayMaterial **)&texture->color1, vector);
            scaleTexture((PovRayMaterial **)&texture->color2, vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovRayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            if (layer->constantFlag) {
                layer = copyTexture(layer);
                texture->layers[i] = layer;
            }
            applyScaleTransform(layer, vector);
            if (layer->textureNumber == (int)CHECKER_TEXTURE_TEXTURE) {
                scaleTexture((PovRayMaterial **)&layer->color1, vector);
                scaleTexture((PovRayMaterial **)&layer->color2, vector);
            }
        }
    }
}
