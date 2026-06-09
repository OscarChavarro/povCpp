#include "render/shaders/PhongSpecularShader.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include <cmath>

void
PhongSpecularShader::shade(Texture *texture, RayWithSegments *lightSourceRay,
    Vector3Dd eye, Vector3Dd *surfaceNormal, ColorRgba *color,
    ColorRgba *lightColor, ColorRgba *surfaceColor)
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
        localNormal = (*surfaceNormal).multiply(-1.0);
    }

    normalProjection = localNormal.multiply(cosAngleOfIncidence);
    normalProjection = normalProjection.multiply(2.0);
    reflectDirection = eye.add(normalProjection);

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
        color->setR(color->getR() + intensity * (surfaceColor->getR()));
        color->setG(color->getG() + intensity * (surfaceColor->getG()));
        color->setB(color->getB() + intensity * (surfaceColor->getB()));
    } else {
        color->setR(color->getR() + intensity * (lightColor->getR()));
        color->setG(color->getG() + intensity * (lightColor->getG()));
        color->setB(color->getB() + intensity * (lightColor->getB()));
    }
}
