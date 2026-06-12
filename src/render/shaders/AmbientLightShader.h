#ifndef __AMBIENT_LIGHT_SHADER_H__
#define __AMBIENT_LIGHT_SHADER_H__

#include "environment/material/Material.h"

class ColorRgba;

class AmbientLightShader {
public:
    static void shade(const Material *texture, const ColorRgba *surfaceColor,
        ColorRgba *color, double attenuation);
};

#endif
