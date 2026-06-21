#ifndef __POV_RAY_MATERIAL_CONSTANCY__
#define __POV_RAY_MATERIAL_CONSTANCY__

#include "java/util/ArrayList.h"

class PovRayMaterial;

// Tracks which PovRayMaterial instances are shared/canonical (the scene's default
// texture, or the backing object of a #declare'd texture identifier) and therefore
// must be copied before the parser mutates them in place (layering, transforms).
class PovRayMaterialConstancy {
  public:
    static void markConstant(const PovRayMaterial *material);
    static bool isConstant(const PovRayMaterial *material);
    // Must be called immediately before deleting a marked-constant PovRayMaterial
    // (e.g. when #declare re-declaring an identifier frees its old backing
    // object) - otherwise the registry keeps a dangling pointer that could later
    // false-positive-match an unrelated PovRayMaterial allocated at the same
    // (now-reused) address.
    static void unmarkConstant(const PovRayMaterial *material);

  private:
    static java::ArrayList<const PovRayMaterial *> &constantMaterials();
};

#endif
