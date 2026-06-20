/**
PovrayMaterial utilities: global default texture management.

PovrayMaterial-side utilities: the global default texture, color-map sampling, the wave
sources/frequencies used by ripple/wave bump textures, and texture-space transforms
(translate/rotate/scale/copy) for POV-Ray material descriptors.
*/

#ifndef __POVRAY_MATERIAL_UTILS__
#define __POVRAY_MATERIAL_UTILS__

#include "environment/material/Material.h"
#include "environment/material/PovRayMaterial.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class PovrayMaterialUtils {
  private:
    static bool needsTransform(const PovRayMaterial *texture);
    static void applyTranslationTransform(
        PovRayMaterial *texture, const Vector3Dd *vector);
    static void applyRotationTransform(
        PovRayMaterial *texture, Vector3Dd *vector);
    static void applyScaleTransform(
        PovRayMaterial *texture, const Vector3Dd *vector);
    static void copyTextureNode(PovRayMaterial *dst, const PovRayMaterial *src);

  public:
    static void translateTexture(
        PovRayMaterial **texturePtr, Vector3Dd *vector);
    static void rotateTexture(PovRayMaterial **texturePtr, Vector3Dd *vector);
    static void scaleTexture(PovRayMaterial **texturePtr, Vector3Dd *vector);
    static void prependTextureLayers(
        PovRayMaterial *newHead, Material *&existingHead);
    static void prependTextureLayers(
        PovRayMaterial *newHead, PovRayMaterial *&existingHead);
    static PovRayMaterial *copyTexture(PovRayMaterial *texture);
    static PovRayMaterial *getTexture();
};

#endif
