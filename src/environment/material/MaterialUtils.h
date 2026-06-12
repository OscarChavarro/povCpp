/**
Material utilities: global default texture management.

Material-side utilities: the global default texture, color-map sampling, the wave
sources/frequencies used by ripple/wave bump textures, and texture-space transforms
(translate/rotate/scale/copy) for POV-Ray material descriptors.
*/

#ifndef __MATERIAL_UTILS_H__
#define __MATERIAL_UTILS_H__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/material/Material.h"

class MaterialUtils {
  private:
    static MaterialUtils* materialInstance;
    MaterialUtils();

    static bool needsTransform(const Material *texture);
    static void applyTranslationTransform(Material *texture, const Vector3Dd *vector);
    static void applyRotationTransform(Material *texture, Vector3Dd *vector);
    static void applyScaleTransform(Material *texture, const Vector3Dd *vector);
    static void copyTextureNode(Material *dst, const Material *src);

  public:
    static void initialize();
    static MaterialUtils& instance();

    static Material *defaultTexture();
    static void setDefaultTexture(Material *texture);
    static void translateTexture(Material **texturePtr, Vector3Dd *vector);
    static void rotateTexture(Material **texturePtr, Vector3Dd *vector);
    static void scaleTexture(Material **texturePtr, Vector3Dd *vector);
    static Material *copyTexture(Material *texture);
    static Material *getTexture();
};

#endif
