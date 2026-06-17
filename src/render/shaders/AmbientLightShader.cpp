#include "render/shaders/AmbientLightShader.h"

void
AmbientLightShader::shade(const PovrayMaterial *texture, const ColorRgba *surfaceColor,
    ColorRgba *color, double attenuation)
{
    if (texture->getObjectAmbient() == 0.0) {
        return;
    }

    color->setR(color->getR() + surfaceColor->getR() * texture->getObjectAmbient() * attenuation);
    color->setG(color->getG() + surfaceColor->getG() * texture->getObjectAmbient() * attenuation);
    color->setB(color->getB() + surfaceColor->getB() * texture->getObjectAmbient() * attenuation);
}
