#ifndef __AMBIENT_LIGHT_SHADER_H__
#define __AMBIENT_LIGHT_SHADER_H__

#include "media/Texture.h"

class RGBAColor;

class AmbientLightShader {
public:
    static void shade(Texture *texture, RGBAColor *surfaceColor,
        RGBAColor *color, double attenuation);
};

#endif
