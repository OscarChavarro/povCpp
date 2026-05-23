#include "render/shaders/BlinnPhongSpecularShader.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include <cmath>

void
BlinnPhongSpecularShader::shade(Texture *texture, RayWithSegments *lightSourceRay,
    Vector3Dd rEye, Vector3Dd *surfaceNormal, RGBAColor *colour,
    RGBAColor *lightColour, RGBAColor *surfaceColour)
{
    double cosAngleOfIncidence, normalLength, intensity, halfwayLength,
        roughness;
    Vector3Dd halfway;

    halfway = rEye.half(lightSourceRay->direction);
    normalLength = (*surfaceNormal).length();
    halfwayLength = halfway.length();
    cosAngleOfIncidence = halfway.dotProduct(*surfaceNormal);

    if (normalLength == 0.0 || halfwayLength == 0.0) {
        cosAngleOfIncidence = 0.0;
    } else {
        cosAngleOfIncidence /= (normalLength * halfwayLength);
    }

    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = 0.0;
    }

    roughness = 1.0 / texture->Object_Roughness;

    if (roughness != 1.0) {
        intensity = pow(cosAngleOfIncidence, roughness);
    } else {
        intensity = cosAngleOfIncidence;
    }
    intensity *= texture->Object_Specular;
    if (texture->Metallic_Flag) {
        colour->Red += intensity * (surfaceColour->Red);
        colour->Green += intensity * (surfaceColour->Green);
        colour->Blue += intensity * (surfaceColour->Blue);
    } else {
        colour->Red += intensity * (lightColour->Red);
        colour->Green += intensity * (lightColour->Green);
        colour->Blue += intensity * (lightColour->Blue);
    }
}
