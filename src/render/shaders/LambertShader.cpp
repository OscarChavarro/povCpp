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

    if (texture->objectBrilliance != 1.0) {
        intensity = pow(cosAngleOfIncidence, texture->objectBrilliance);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->objectDiffuse * attenuation;

    randomNumber = (rand() & 0x7FFF) / (double)0x7FFF;

    intensity -= randomNumber * texture->textureRandomness;

    color->Red += intensity * (surfaceColor->Red) * (lightColor->Red);
    color->Green += intensity * (surfaceColor->Green) * (lightColor->Green);
    color->Blue += intensity * (surfaceColor->Blue) * (lightColor->Blue);
}
