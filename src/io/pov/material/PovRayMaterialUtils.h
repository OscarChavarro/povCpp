/**
PovrayMaterial utilities: global default texture management.

PovrayMaterial-side utilities: the global default texture, color-map sampling, the wave
sources/frequencies used by ripple/wave bump textures, and texture-space transforms
(translate/rotate/scale/copy) for POV-Ray material descriptors.
*/

#ifndef __POV_RAY_MATERIAL_UTILS__
#define __POV_RAY_MATERIAL_UTILS__

#include "environment/material/povray/PovRayMaterial.h"

class PovRayMaterialUtils {
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
