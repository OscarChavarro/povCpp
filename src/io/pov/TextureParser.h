#ifndef __TEXTURE_PARSER_H__
#define __TEXTURE_PARSER_H__

class Texture;

class TextureParser {
  public:
    static Texture *parseTexture();
    static Texture *copyTexture(Texture *texture);
};

#endif
