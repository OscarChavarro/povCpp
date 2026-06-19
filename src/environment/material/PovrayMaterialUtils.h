/**
PovrayMaterial utilities: global default texture management.

PovrayMaterial-side utilities: the global default texture, color-map sampling, the wave
sources/frequencies used by ripple/wave bump textures, and texture-space transforms
(translate/rotate/scale/copy) for POV-Ray material descriptors.
*/

#ifndef __POVRAY_MATERIAL_UTILS__
#define __POVRAY_MATERIAL_UTILS__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/PovrayMaterial.h"

class PovrayMaterialUtils {
  private:
    static bool needsTransform(const PovrayMaterial *texture);
    static void applyTranslationTransform(PovrayMaterial *texture, const Vector3Dd *vector);
    static void applyRotationTransform(PovrayMaterial *texture, Vector3Dd *vector);
    static void applyScaleTransform(PovrayMaterial *texture, const Vector3Dd *vector);
    static void copyTextureNode(PovrayMaterial *dst, const PovrayMaterial *src);

  public:
    static void translateTexture(PovrayMaterial **texturePtr, Vector3Dd *vector);
    static void rotateTexture(PovrayMaterial **texturePtr, Vector3Dd *vector);
    static void scaleTexture(PovrayMaterial **texturePtr, Vector3Dd *vector);
    static PovrayMaterial *copyTexture(PovrayMaterial *texture);
    static PovrayMaterial *getTexture();
};

#endif
