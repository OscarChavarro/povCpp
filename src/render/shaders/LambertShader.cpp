#include "render/shaders/LambertShader.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include <cstdlib>
#include <cmath>

void
LambertShader::shade(Texture *texture, RayWithSegments *lightSourceRay,
    Vector3Dd *surfaceNormal, RGBAColor *colour, RGBAColor *lightColour,
    RGBAColor *surfaceColour, double attenuation)
{
    double cosAngleOfIncidence, intensity, randomNumber;

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

    colour->Red += intensity * (surfaceColour->Red) * (lightColour->Red);
    colour->Green += intensity * (surfaceColour->Green) * (lightColour->Green);
    colour->Blue += intensity * (surfaceColour->Blue) * (lightColour->Blue);
}
