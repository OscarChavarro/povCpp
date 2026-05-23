#include "render/shaders/PhongSpecularShader.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include <cmath>

void
PhongSpecularShader::shade(Texture *texture, RayWithSegments *lightSourceRay,
    Vector3Dd eye, Vector3Dd *surfaceNormal, RGBAColor *colour,
    RGBAColor *lightColour, RGBAColor *surfaceColour)
{
    double cosAngleOfIncidence, normalLength, intensity;
    Vector3Dd localNormal;
    Vector3Dd normalProjection;
    Vector3Dd reflectDirection;

    cosAngleOfIncidence = eye.dotProduct(*surfaceNormal);

    if (cosAngleOfIncidence < 0.0) {
        localNormal = *surfaceNormal;
        cosAngleOfIncidence = -cosAngleOfIncidence;
    } else {
        VectorOps::vScale(localNormal, *surfaceNormal, -1.0);
    }

    VectorOps::vScale(normalProjection, localNormal, cosAngleOfIncidence);
    normalProjection.scale(2.0);
    VectorOps::vAdd(reflectDirection, eye, normalProjection);

    cosAngleOfIncidence =
        reflectDirection.dotProduct(lightSourceRay->direction);
    normalLength = lightSourceRay->direction.length();

    if (normalLength == 0.0) {
        cosAngleOfIncidence = 0.0;
    } else {
        cosAngleOfIncidence /= normalLength;
    }

    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = 0;
    }

    if (texture->Object_PhongSize != 1.0) {
        intensity = pow(cosAngleOfIncidence, texture->Object_PhongSize);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->Object_Phong;

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
