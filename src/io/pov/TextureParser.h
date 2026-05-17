#ifndef __TEXTURE_PARSER_H__
#define __TEXTURE_PARSER_H__

class Texture;
class RGBAColorPalette;

class TextureParser {
  public:
    static Texture *parseTexture();
    static RGBAColorPalette *parseColourMap();
    static Texture *copyTexture(Texture *texture);
};

#endif
