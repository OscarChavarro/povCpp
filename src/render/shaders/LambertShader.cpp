#include "render/shaders/LambertShader.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include <cstdlib>
#include <cmath>

void
LambertShader::shade(Texture *texture, RayWithSegments *lightSourceRay,
    Vector3Dd *surfaceNormal, RGBAColor *color, RGBAColor *lightColor,
    RGBAColor *surfaceColor, double attenuation)
{
    double cosAngleOfIncidence;
    double intensity;
    double randomNumber;

    cosAngleOfIncidence =
        (*surfaceNormal).dotProduct(lightSourceRay->direction);
    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = -cosAngleOfIncidence;
    }

    if (texture->Object_Brilliance != 1.0) {
        intensity = pow(cosAngleOfIncidence, texture->Object_Brilliance);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->Object_Diffuse * attenuation;

    randomNumber = (rand() & 0x7FFF) / (double)0x7FFF;

    intensity -= randomNumber * texture->Texture_Randomness;

    color->Red += intensity * (surfaceColor->Red) * (lightColor->Red);
    color->Green += intensity * (surfaceColor->Green) * (lightColor->Green);
    color->Blue += intensity * (surfaceColor->Blue) * (lightColor->Blue);
}
