/**
Material utilities: global default texture management.
*/

#ifndef __MATERIAL_UTILS_H__
#define __MATERIAL_UTILS_H__

class Material;

class MaterialUtils {
  public:
    static void initialize();
    static MaterialUtils& instance();

    Material *defaultTexture();
    void setDefaultTexture(Material *texture);

  private:
    static MaterialUtils* materialInstance;
    MaterialUtils();
};

#endif
