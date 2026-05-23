#include "render/shaders/AmbientLightShader.h"
#include "common/color/Color.h"

void
AmbientLightShader::shade(Texture *texture, RGBAColor *surfaceColor,
    RGBAColor *color, double attenuation)
{
    if (texture->objectAmbient == 0.0) {
        return;
    }

    color->Red += surfaceColor->Red * texture->objectAmbient * attenuation;
    color->Green +=
        surfaceColor->Green * texture->objectAmbient * attenuation;
    color->Blue += surfaceColor->Blue * texture->objectAmbient * attenuation;
}
