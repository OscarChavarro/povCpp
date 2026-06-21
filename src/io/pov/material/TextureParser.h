#ifndef __TEXTURE_PARSER__
#define __TEXTURE_PARSER__

#include "environment/material/povray/PovRayMaterial.h"
#include "io/pov/context/ParserContext.h"

class PovRayMaterialBuilder;

class TextureParser {
  public:
    static PovRayMaterial *parseTexture();
    static PovRayMaterial *parseTexture(ParserContext &ctx);
    static PovRayMaterial *parseTexture(
        PovRayMaterial *baseTexture, ParserContext &ctx);
    static PovRayMaterial *copyTexture(PovRayMaterial *texture);

  private:
    static PovRayMaterialBuilder editorFor(PovRayMaterial *texture);
    static void wireIndexedInToTextureImage(class ControlledRGBAImageHDRUncompressed *ti, class IndexedColorImageHDRUncompressed *idx);

    // TextureParser rebuilds the whole PovRayMaterial after every single
    // attribute token, discarding the previous generation. After the
    // aliasing audit (see doc/memoryAudit/ownership.md), every rebuilt
    // generation owns its own pigment/normal/colorMap/layers - the only
    // thing the old generation could still share with a sibling is being
    // itself the registered "constant" backing a #declare'd identifier or
    // the scene's default texture, which nothing here is allowed to delete.
    static PovRayMaterial *rebuildAndDiscard(
        PovRayMaterial *oldTexture, const PovRayMaterialBuilder &builder);
};

#endif
