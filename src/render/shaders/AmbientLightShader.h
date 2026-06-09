#ifndef __AMBIENT_LIGHT_SHADER_H__
#define __AMBIENT_LIGHT_SHADER_H__

#include "media/solidTexture/Texture.h"

class ColorRgba;

class AmbientLightShader {
public:
    static void shade(Texture *texture, ColorRgba *surfaceColor,
        ColorRgba *color, double attenuation);
};

#endif
