#include "render/shaders/AmbientLightShader.h"
#include "common/color/Color.h"

void
AmbientLightShader::shade(Texture *texture, RGBAColor *surfaceColour,
    RGBAColor *colour, double attenuation)
{
    if (texture->Object_Ambient == 0.0) {
        return;
    }

    colour->Red += surfaceColour->Red * texture->Object_Ambient * attenuation;
    colour->Green +=
        surfaceColour->Green * texture->Object_Ambient * attenuation;
    colour->Blue += surfaceColour->Blue * texture->Object_Ambient * attenuation;
}
