/**
PovrayMaterial utilities: global default texture management.

PovrayMaterial-side utilities: the global default texture, color-map sampling, the wave
sources/frequencies used by ripple/wave bump textures, and texture-space transforms
(translate/rotate/scale/copy) for POV-Ray material descriptors.
*/

#ifndef __POVRAY_MATERIAL_UTILS__
#define __POVRAY_MATERIAL_UTILS__

#include "environment/material/Material.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

// Thin facade over PovRayMaterial's own copy/transform engine. The actual writes to the
// (now immutable-by-default) PovRayMaterial state live in PovRayMaterial member functions,
// so this class holds no privileged access and contains no setters.
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
