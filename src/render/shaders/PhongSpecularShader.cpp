#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "environment/geometry/element/RayWithTracingState.h"
#include "render/shaders/PhongSpecularShader.h"

void
PhongSpecularShader::shade(const PovRayMaterial *texture, const RayWithTracingState *lightSourceRay,
    Vector3Dd eye, const Vector3Dd *surfaceNormal, ColorRgba *color,
    const ColorRgba *lightColor, const ColorRgba *surfaceColor)
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
        reflectDirection.dotProduct(lightSourceRay->getDirection());
    normalLength = lightSourceRay->getDirection().length();

    if (normalLength == 0.0) {
        cosAngleOfIncidence = 0.0;
    } else {
        cosAngleOfIncidence /= normalLength;
    }

    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = 0;
    }

    if (texture->getObjectPhongSize() != 1.0) {
        intensity = java::Math::pow(cosAngleOfIncidence, texture->getObjectPhongSize());
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->getObjectPhong();

    if (texture->isMetallic()) {
        color->setR(color->getR() + intensity * (surfaceColor->getR()));
        color->setG(color->getG() + intensity * (surfaceColor->getG()));
        color->setB(color->getB() + intensity * (surfaceColor->getB()));
    } else {
        color->setR(color->getR() + intensity * (lightColor->getR()));
        color->setG(color->getG() + intensity * (lightColor->getG()));
        color->setB(color->getB() + intensity * (lightColor->getB()));
    }
}
