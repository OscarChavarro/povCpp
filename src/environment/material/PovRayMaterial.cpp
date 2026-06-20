#include "environment/material/PovRayMaterial.h"
#include "environment/material/pigment/SolidTexturePigment.h"
#include "environment/material/normal/SolidTextureNormal.h"
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"

PovRayMaterial::PovRayMaterial(
    double objReflection, double objAmbient, double objDiffuse,
    double objBrilliance, double objIndexOfRefraction, double objRefraction,
    double objTransmit, double objSpecular, double objRoughness,
    double objPhong, double objPhongSize, double texRandomness,
    Matrix4x4d *texTransform, Matrix4x4d *texTransformInv,
    SolidTexturePigment *pig, SolidTextureNormal *norm,
    ControlledRGBAImageHDRUncompressed *materialImg, bool metallic,
    java::ArrayList<PovRayMaterial *> layersList,
    java::ArrayList<PovRayMaterial *> materialsList,
    double pendingTurb, int pendingOct, RGBAColorPalette *pendingMap,
    double pendingMort, double pendingBump, double pendingFreq,
    double pendingPh, int pendingWaves,
    ControlledRGBAImageHDRUncompressed *pendingBumpImg) :
    pendingBumpAmount(pendingBump),
    pendingFrequency(pendingFreq),
    pendingMortar(pendingMort),
    pendingPhase(pendingPh),
    pendingTurbulence(pendingTurb),
    pendingNumberOfWaves(pendingWaves),
    pendingOctaves(pendingOct),
    pendingBumpImage(pendingBumpImg),
    pendingColorMap(pendingMap),
    layers(layersList),
    materialMapImage(materialImg),
    materialMapVariants(materialsList),
    metallicFlag(metallic),
    objectAmbient(objAmbient),
    objectBrilliance(objBrilliance),
    objectDiffuse(objDiffuse),
    objectIndexOfRefraction(objIndexOfRefraction),
    objectPhong(objPhong),
    objectPhongSize(objPhongSize),
    objectReflection(objReflection),
    objectRefraction(objRefraction),
    objectRoughness(objRoughness),
    objectSpecular(objSpecular),
    objectTransmit(objTransmit),
    pigment(pig),
    normal(norm),
    textureRandomness(texRandomness),
    textureTransformation(texTransform),
    textureTransformationInverse(texTransformInv)
{
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

PovRayMaterial *
PovRayMaterial::copy()
{
    return copyTexture(this);
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

PovRayMaterial *
PovRayMaterial::copyTextureNode(const PovRayMaterial *src)
{
    Matrix4x4d *transformation = nullptr;
    Matrix4x4d *transformationInverse = nullptr;
    if (src->textureTransformation != nullptr) {
        transformation = new Matrix4x4d(*src->textureTransformation);
        transformationInverse = new Matrix4x4d(*src->textureTransformationInverse);
    }
    SolidTexturePigment *newPigment = (src->pigment != nullptr) ? src->pigment->copy() : nullptr;
    SolidTextureNormal *newNormal = (src->normal != nullptr) ? src->normal->copy() : nullptr;
    return new PovRayMaterial(
        src->objectReflection, src->objectAmbient, src->objectDiffuse,
        src->objectBrilliance, src->objectIndexOfRefraction, src->objectRefraction,
        src->objectTransmit, src->objectSpecular, src->objectRoughness,
        src->objectPhong, src->objectPhongSize, src->textureRandomness,
        transformation, transformationInverse, newPigment, newNormal,
        src->materialMapImage, src->metallicFlag,
        src->layers, src->materialMapVariants,
        src->pendingTurbulence, src->pendingOctaves,
        SolidTexturePigment::cloneColorMap(src->pendingColorMap),
        src->pendingMortar, src->pendingBumpAmount, src->pendingFrequency,
        src->pendingPhase, src->pendingNumberOfWaves, src->pendingBumpImage);
}

// ---------------------------------------------------------------------------
// Texture-space copy/transform engine (own member functions; the only writers
// of the private state besides the constructors). Parse-time only.
// ---------------------------------------------------------------------------

bool
PovRayMaterial::needsTransform(const PovRayMaterial *texture)
{
    return ((texture->pigment != nullptr) && texture->pigment->needsTransform()) ||
           (texture->normal != nullptr);
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

Material *
PovRayMaterial::rotate(Vector3Dd *vector)
{
    PovRayMaterial *self = this;
    rotateTexture(&self, vector);
    return this;
}

void
PovRayMaterial::rotateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        applyRotationTransform(texture, vector);
        if (texture->pigment != nullptr) {
            texture->pigment->rotate(vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovRayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            applyRotationTransform(layer, vector);
            if (layer->pigment != nullptr) {
                layer->pigment->rotate(vector);
            }
        }
    }
}

Material *
PovRayMaterial::scale(Vector3Dd *vector)
{
    PovRayMaterial *self = this;
    scaleTexture(&self, vector);
    return this;
}

void
PovRayMaterial::scaleTexture(PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        applyScaleTransform(texture, vector);
        if (texture->pigment != nullptr) {
            texture->pigment->scale(vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovRayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            applyScaleTransform(layer, vector);
            if (layer->pigment != nullptr) {
                layer->pigment->scale(vector);
            }
        }
    }
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

void
PovRayMaterial::translateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial *texture = *texturePtr;
    if (texture == nullptr) {
        return;
    }

    if (needsTransform(texture)) {
        applyTranslationTransform(texture, vector);
        if (texture->pigment != nullptr) {
            texture->pigment->translate(vector);
        }
    }

    for (long int i = 0; i < texture->layers.size(); i++) {
        PovRayMaterial *layer = texture->layers[i];
        if (needsTransform(layer)) {
            applyTranslationTransform(layer, vector);
            if (layer->pigment != nullptr) {
                layer->pigment->translate(vector);
            }
        }
    }
}
