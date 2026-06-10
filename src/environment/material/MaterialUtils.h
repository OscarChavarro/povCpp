/**
Material utilities: global default texture management.
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

  public:
    static void initialize();
    static MaterialUtils& instance();

    static Material *defaultTexture();
    static void setDefaultTexture(Material *texture);
    void translateTexture(Material **texturePtr, Vector3Dd *vector);
    void rotateTexture(Material **texturePtr, Vector3Dd *vector);
    void scaleTexture(Material **texturePtr, Vector3Dd *vector);
    Material *copyTexture(Material *texture);
    Material *getTexture();
};

#endif
