/**
PovrayMaterial utilities: global default texture management.

Thin facade delegating to PovRayMaterial's own copy/transform engine.
*/

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/common/logging/Logger.h"

#include "environment/material/PovRayMaterial.h"
#include "environment/material/PovRayMaterialUtils.h"

void
PovRayMaterialUtils::translateTexture(
    PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial::translateTexture(texturePtr, vector);
}

void
PovRayMaterialUtils::rotateTexture(
    PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial::rotateTexture(texturePtr, vector);
}

void
PovRayMaterialUtils::scaleTexture(
    PovRayMaterial **texturePtr, Vector3Dd *vector)
{
    PovRayMaterial::scaleTexture(texturePtr, vector);
}

void
PovRayMaterialUtils::prependTextureLayers(
    PovRayMaterial *newHead, Material *&existingHead)
{
    PovRayMaterial *existingPovrayHead = static_cast<PovRayMaterial *>(existingHead);
    PovRayMaterial::prependTextureLayers(newHead, existingPovrayHead);
    existingHead = existingPovrayHead;
}

void
PovRayMaterialUtils::prependTextureLayers(
    PovRayMaterial *newHead, PovRayMaterial *&existingHead)
{
    PovRayMaterial::prependTextureLayers(newHead, existingHead);
}

PovRayMaterial *
PovRayMaterialUtils::copyTexture(PovRayMaterial *texture)
{
    return PovRayMaterial::copyTexture(texture);
}

PovRayMaterial *
PovRayMaterialUtils::getTexture()
{
    PovRayMaterial * const newTexture = new PovRayMaterial;
    if (newTexture == nullptr) {
        Logger::reportMessage("PovrayMaterial", Logger::FATAL_ERROR, "", "Out of memory. Cannot allocate object");
    }
    return (newTexture);
}
