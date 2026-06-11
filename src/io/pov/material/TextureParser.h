#ifndef __TEXTURE_PARSER_H__
#define __TEXTURE_PARSER_H__

class Material;
class ParserContext;

class TextureParser {
  public:
    static Material *parseTexture();
    static Material *parseTexture(ParserContext &ctx);
    static Material *copyTexture(Material *texture);
    static void prependTextureLayers(Material *newHead, Material *&existingHead);

  private:
    static bool shouldLogTextureState();
    static void logTextureStateLegacy(const char *prefix, const Material *texture);
    static void wireIndexedInToTextureImage(class ControlledRGBAImageHDRUncompressed *ti, class IndexedColorImageHDRUncompressed *idx);
};

#endif
