#ifndef __TEXTURE_PARSER__
#define __TEXTURE_PARSER__

#include "environment/material/PovrayMaterial.h"
#include "io/pov/context/ParserContext.h"

class TextureParser {
  public:
    static PovrayMaterial *parseTexture();
    static PovrayMaterial *parseTexture(ParserContext &ctx);
    static PovrayMaterial *parseTexture(PovrayMaterial *baseTexture, ParserContext &ctx);
    static PovrayMaterial *copyTexture(PovrayMaterial *texture);

  private:
    static PovrayMaterial *ensureWritableTexture(PovrayMaterial *texture);
    static void wireIndexedInToTextureImage(class ControlledRGBAImageHDRUncompressed *ti, class IndexedColorImageHDRUncompressed *idx);
};

#endif
