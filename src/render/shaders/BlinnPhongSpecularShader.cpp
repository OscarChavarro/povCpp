#include "java/lang/Math.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

#include "environment/geometry/element/RayWithSegments.h"

#include "render/shaders/BlinnPhongSpecularShader.h"

void
BlinnPhongSpecularShader::shade(const PovrayMaterial *texture, const RayWithSegments *lightSourceRay,
    Vector3Dd rEye, const Vector3Dd *surfaceNormal, ColorRgba *color,
    const ColorRgba *lightColor, const ColorRgba *surfaceColor)
{
    double cosAngleOfIncidence;
    double normalLength;
    double intensity;
    double halfwayLength;
    double roughness;
    Vector3Dd halfway;

    halfway = rEye.midpoint(lightSourceRay->getDirection());
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
        intensity = java::Math::pow(cosAngleOfIncidence, roughness);
    } else {
        intensity = cosAngleOfIncidence;
    }
    intensity *= texture->objectSpecular;
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
