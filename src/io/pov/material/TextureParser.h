#ifndef __TEXTURE_PARSER__
#define __TEXTURE_PARSER__

#include "environment/material/povray/PovRayMaterial.h"
#include "environment/material/povray/PovRayMaterialBuilder.h"
#include "io/pov/context/ParserContext.h"
#include "vsdk/toolkit/media/IndexedColorImageHDRUncompressed.h"
#include "vsdk/toolkit/media/solidTexture/from2d/ControlledRGBAImageHDRUncompressed.h"

class TextureParser {
  public:
    static PovRayMaterial *parseTexture();
    static PovRayMaterial *parseTexture(ParserContext &ctx);
    static PovRayMaterial *parseTexture(
        PovRayMaterial *baseTexture, ParserContext &ctx);
    static PovRayMaterial *copyTexture(PovRayMaterial *texture);

  private:
    static PovRayMaterialBuilder editorFor(PovRayMaterial *texture);
    static void wireIndexedInToTextureImage(
        ControlledRGBAImageHDRUncompressed *ti,
        IndexedColorImageHDRUncompressed *idx);
    static PovRayMaterial *rebuildAndDiscard(
        PovRayMaterial *oldTexture, const PovRayMaterialBuilder &builder);
};

#endif
