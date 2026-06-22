#ifndef __TEXTURE_ALIAS_ENTRY__
#define __TEXTURE_ALIAS_ENTRY__

#include "environment/material/povray/PovRayMaterial.h"

class TextureAliasEntry {
  private:
    PovRayMaterial *material;
    int count;
    bool retired;

  public:
    TextureAliasEntry() : material(nullptr), count(0), retired(false) {}

    explicit TextureAliasEntry(PovRayMaterial *material)
        : material(material), count(1), retired(false) {}

    PovRayMaterial *getMaterial() const { return material; }
    int getCount() const { return count; }
    bool isRetired() const { return retired; }

    void incrementCount() { count++; }
    void decrementCount() { count--; }
    void markRetired() { retired = true; }
};

#endif
