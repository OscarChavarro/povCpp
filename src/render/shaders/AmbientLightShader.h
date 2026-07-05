#ifndef __AMBIENT_LIGHT_SHADER__
#define __AMBIENT_LIGHT_SHADER__

#include "environment/material/povray/PovRayMaterial.h"

class AmbientLightShader {
public:
    static void shade(const PovRayMaterial *texture, const ColorRgba *surfaceColor,
        ColorRgba *color, double attenuation);
};

#endif
