#ifndef __DEFAULT_TEXTURE_ALIAS_TRACKER__
#define __DEFAULT_TEXTURE_ALIAS_TRACKER__

#include "java/util/ArrayList.h"
#include "environment/material/povray/PovRayMaterial.h"
#include "environment/material/povray/TextureAliasEntry.h"

class DefaultTextureAliasTracker {
  private:
    static java::ArrayList<TextureAliasEntry> &entries();
    static long int findEntryIndex(PovRayMaterial *material);

  public:
    static void trackAlias(PovRayMaterial *defaultTexture);
    static void releaseAlias(PovRayMaterial *material);
    static void retire(PovRayMaterial *oldDefaultTexture);
};

#endif
