#ifndef __AMBIENT_LIGHT_SHADER__
#define __AMBIENT_LIGHT_SHADER__

#include "vsdk/toolkit/common/color/ColorRgba.h"
#include "environment/material/PovrayMaterial.h"

class AmbientLightShader {
public:
    static void shade(const PovrayMaterial *texture, const ColorRgba *surfaceColor,
        ColorRgba *color, double attenuation);
};

#endif
