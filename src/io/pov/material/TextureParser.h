#ifndef __TEXTURE_PARSER_H__
#define __TEXTURE_PARSER_H__

#include "environment/material/Material.h"
#include "environment/material/PovrayMaterial.h"
#include "io/pov/context/ParserContext.h"

class TextureParser {
  public:
    static PovrayMaterial *parseTexture();
    static PovrayMaterial *parseTexture(ParserContext &ctx);
    static PovrayMaterial *copyTexture(PovrayMaterial *texture);
    static void prependTextureLayers(PovrayMaterial *newHead, Material *&existingHead);
    static void prependTextureLayers(PovrayMaterial *newHead, PovrayMaterial *&existingHead);

  private:
    static bool shouldLogTextureState();
    static void logTextureStateLegacy(const char *prefix, const PovrayMaterial *texture);
    static void wireIndexedInToTextureImage(class ControlledRGBAImageHDRUncompressed *ti, class IndexedColorImageHDRUncompressed *idx);
};

#endif
