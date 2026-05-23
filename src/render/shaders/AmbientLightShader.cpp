#include "render/shaders/AmbientLightShader.h"
#include "common/color/Color.h"

void
AmbientLightShader::shade(Texture *texture, RGBAColor *surfaceColor,
    RGBAColor *color, double attenuation)
{
    if (texture->Object_Ambient == 0.0) {
        return;
    }

    color->Red += surfaceColor->Red * texture->Object_Ambient * attenuation;
    color->Green +=
        surfaceColor->Green * texture->Object_Ambient * attenuation;
    color->Blue += surfaceColor->Blue * texture->Object_Ambient * attenuation;
}
