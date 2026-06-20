#ifndef __TEXTURE_PARSER__
#define __TEXTURE_PARSER__

#include "environment/material/Material.h"
#include "environment/material/PovrayMaterial.h"
#include "io/pov/context/ParserContext.h"

class SimpleBody;

class TextureParser {
  public:
    static PovrayMaterial *parseTexture();
    static PovrayMaterial *parseTexture(ParserContext &ctx);
    static PovrayMaterial *parseTexture(PovrayMaterial *baseTexture, ParserContext &ctx);
    static PovrayMaterial *copyTexture(PovrayMaterial *texture);
    static void prependTextureLayers(PovrayMaterial *newHead, SimpleBody *body);
    static void prependTextureLayers(PovrayMaterial *newHead, Material *&existingHead);
    static void prependTextureLayers(PovrayMaterial *newHead, PovrayMaterial *&existingHead);

  private:
    static PovrayMaterial *ensureWritableTexture(PovrayMaterial *texture);
    static void wireIndexedInToTextureImage(class ControlledRGBAImageHDRUncompressed *ti, class IndexedColorImageHDRUncompressed *idx);
};

#endif
