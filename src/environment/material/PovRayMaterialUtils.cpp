#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/PovRayMaterial.h"
#include "environment/material/PovRayMaterialUtils.h"
#include "io/pov/material/PovRayMaterialBuilder.h"
#include "io/pov/material/PovRayMaterialConstancy.h"

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
    PovRayMaterial *existingPovRayHead = static_cast<PovRayMaterial *>(existingHead);
    PovRayMaterial::prependTextureLayers(newHead, existingPovRayHead);
    existingHead = existingPovRayHead;
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
    PovRayMaterial *texture = PovRayMaterialBuilder().build();
    PovRayMaterialConstancy::markConstant(texture);
    return texture;
}
