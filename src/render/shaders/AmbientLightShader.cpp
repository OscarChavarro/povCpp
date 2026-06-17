#include "render/shaders/AmbientLightShader.h"

void
AmbientLightShader::shade(const PovrayMaterial *texture, const ColorRgba *surfaceColor,
    ColorRgba *color, double attenuation)
{
    if (texture->objectAmbient == 0.0) {
        return;
    }

    color->setR(color->getR() + surfaceColor->getR() * texture->objectAmbient * attenuation);
    color->setG(color->getG() + surfaceColor->getG() * texture->objectAmbient * attenuation);
    color->setB(color->getB() + surfaceColor->getB() * texture->objectAmbient * attenuation);
}
