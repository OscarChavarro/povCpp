#include "environment/material/povray/DefaultTextureAliasTracker.h"
#include "environment/material/povray/PovRayMaterialConstancy.h"
#include "java/util/ArrayList.txx"

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
    quickColor(nullptr),
    textureRandomness(texRandomness),
    textureTransformation(texTransform),
    textureTransformationInverse(texTransformInv)
{
}

PovRayMaterial::PovRayMaterial(const PovRayMaterial &other) :
    pendingBumpAmount(other.pendingBumpAmount),
    pendingFrequency(other.pendingFrequency),
    pendingMortar(other.pendingMortar),
    pendingPhase(other.pendingPhase),
    pendingTurbulence(other.pendingTurbulence),
    pendingNumberOfWaves(other.pendingNumberOfWaves),
    pendingOctaves(other.pendingOctaves),
    pendingBumpImage(other.pendingBumpImage),
    pendingColorMap(SolidTexturePigment::cloneColorMap(other.pendingColorMap)),
    layers(),
    materialMapImage(other.materialMapImage),
    materialMapVariants(),
    metallicFlag(other.metallicFlag),
    objectAmbient(other.objectAmbient),
    objectBrilliance(other.objectBrilliance),
    objectDiffuse(other.objectDiffuse),
    objectIndexOfRefraction(other.objectIndexOfRefraction),
    objectPhong(other.objectPhong),
    objectPhongSize(other.objectPhongSize),
    objectReflection(other.objectReflection),
    objectRefraction(other.objectRefraction),
    objectRoughness(other.objectRoughness),
    objectSpecular(other.objectSpecular),
    objectTransmit(other.objectTransmit),
    pigment(other.pigment != nullptr ? other.pigment->copy() : nullptr),
    normal(other.normal != nullptr ? other.normal->copy() : nullptr),
    quickColor(nullptr),
    textureRandomness(other.textureRandomness),
    textureTransformation(other.textureTransformation != nullptr ?
        new Matrix4x4d(*other.textureTransformation) : nullptr),
    textureTransformationInverse(other.textureTransformationInverse != nullptr ?
        new Matrix4x4d(*other.textureTransformationInverse) : nullptr)
{
    for (long int i = 0; i < other.layers.size(); i++) {
        layers.add(new PovRayMaterial(*other.layers[i]));
    }
    for (long int i = 0; i < other.materialMapVariants.size(); i++) {
        materialMapVariants.add(new PovRayMaterial(*other.materialMapVariants[i]));
    }
    if (other.quickColor != nullptr) {
        quickColor = new ColorRgba(*other.quickColor);
    }
}

PovRayMaterial::~PovRayMaterial()
{
    delete pigment;
    delete normal;
    delete quickColor;
    delete textureTransformation;
    delete textureTransformationInverse;
    delete pendingColorMap;
    // pendingBumpImage/materialMapImage are not owned here: they are allocated once
    // per image_map{}/bump_map{}/material_map{} occurrence in the .pov source and
    // threaded through every rebuild generation by reference (see ownership.md) -
    // exactly one ImageMapPigment/BumpMapNormal/material_map consumer is responsible
    // for them, never PovRayMaterial itself.
    for (long int i = 0; i < layers.size(); i++) {
        delete layers[i];
    }
    for (long int i = 0; i < materialMapVariants.size(); i++) {
        delete materialMapVariants[i];
    }
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

ICheckerTextureSlot *
PovRayMaterial::copySlot() const
{
    return new PovRayMaterial(*this);
}

ColorRgba *
PovRayMaterial::ensureQuickColor()
{
    if (quickColor == nullptr) {
        quickColor = new ColorRgba(0.0, 0.0, 0.0, 0.0);
    }
    return quickColor;
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

PovRayMaterial *
PovRayMaterial::prependMaterialLayers(PovRayMaterial *existingMaterial)
{
    if (existingMaterial != nullptr && existingMaterial != this) {
        layers.add(existingMaterial);
        for (long int i = 0; i < existingMaterial->layers.size(); i++) {
            layers.add(existingMaterial->layers[i]);
        }
        existingMaterial->layers.clear();
    }
    return this;
}

void
PovRayMaterial::releaseFromOwner()
{
    // An owning object's objectTexture starts out as the scene's shared default
    // texture (see ObjectParser::parseObject/parseComposite) and stays that way
    // for any untextured object - never delete the registered-constant instance,
    // only a private copy of it. If it's still aliasing the (possibly already
    // superseded) default texture, tell DefaultTextureAliasTracker this alias
    // just ended - a safe no-op if it isn't the tracked default (e.g. some other
    // #declare'd TEXTURE_CONSTANT).
    if (!PovRayMaterialConstancy::isConstant(this)) {
        delete this;
    } else {
        DefaultTextureAliasTracker::releaseAlias(this);
    }
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
PovRayMaterial::copy() const
{
    return new PovRayMaterial(*this);
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
