#include "render/shaders/BlinnPhongSpecularShader.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include <cmath>

void
BlinnPhongSpecularShader::shade(Texture *texture, RayWithSegments *lightSourceRay,
    Vector3Dd rEye, Vector3Dd *surfaceNormal, RGBAColor *color,
    RGBAColor *lightColor, RGBAColor *surfaceColor)
{
    double cosAngleOfIncidence;
    double normalLength;
    double intensity;
    double halfwayLength;
    double roughness;
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

    roughness = 1.0 / texture->objectRoughness;

    if (roughness != 1.0) {
        intensity = pow(cosAngleOfIncidence, roughness);
    } else {
        intensity = cosAngleOfIncidence;
    }
    intensity *= texture->objectSpecular;
    if (texture->metallicFlag) {
        color->Red += intensity * (surfaceColor->Red);
        color->Green += intensity * (surfaceColor->Green);
        color->Blue += intensity * (surfaceColor->Blue);
    } else {
        color->Red += intensity * (lightColor->Red);
        color->Green += intensity * (lightColor->Green);
        color->Blue += intensity * (lightColor->Blue);
    }
}
