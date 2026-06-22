#ifndef __POV_RAY_MATERIAL_CONSTANCY__
#define __POV_RAY_MATERIAL_CONSTANCY__

#include "java/util/ArrayList.h"
#include "environment/material/povray/PovRayMaterial.h"

class PovRayMaterialConstancy {
  public:
    static void markConstant(const PovRayMaterial *material);
    static bool isConstant(const PovRayMaterial *material);
    static void unmarkConstant(const PovRayMaterial *material);

  private:
    static java::ArrayList<const PovRayMaterial *> &constantMaterials();
};

#endif
