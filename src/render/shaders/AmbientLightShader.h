#ifndef __AMBIENT_LIGHT_SHADER_H__
#define __AMBIENT_LIGHT_SHADER_H__

#include "solidTexture/Material.h"

class ColorRgba;

class AmbientLightShader {
public:
    static void shade(Material *texture, ColorRgba *surfaceColor,
        ColorRgba *color, double attenuation);
};

#endif
