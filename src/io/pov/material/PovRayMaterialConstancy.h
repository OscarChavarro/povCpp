#ifndef __POV_RAY_MATERIAL_CONSTANCY__
#define __POV_RAY_MATERIAL_CONSTANCY__

class PovRayMaterial;

// Tracks which PovRayMaterial instances are shared/canonical (the scene's default
// texture, or the backing object of a #declare'd texture identifier) and therefore
// must be copied before the parser mutates them in place (layering, transforms).
class PovRayMaterialConstancy {
  public:
    static void markConstant(const PovRayMaterial *material);
    static bool isConstant(const PovRayMaterial *material);
};

#endif
