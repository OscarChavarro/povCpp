#include "render/shaders/PhongSpecularShader.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include <cmath>

void
PhongSpecularShader::shade(Texture *texture, RayWithSegments *lightSourceRay,
    Vector3Dd eye, Vector3Dd *surfaceNormal, RGBAColor *color,
    RGBAColor *lightColor, RGBAColor *surfaceColor)
{
    double cosAngleOfIncidence;
    double normalLength;
    double intensity;
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

    if (texture->objectPhongSize != 1.0) {
        intensity = pow(cosAngleOfIncidence, texture->objectPhongSize);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->objectPhong;

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
