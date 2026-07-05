#include "java/lang/Math.h"
#include "render/shaders/BlinnPhongSpecularShader.h"

void
BlinnPhongSpecularShader::shade(const PovRayMaterial *texture, const RayWithTracingState *lightSourceRay,
    Vector3Dd rEye, const Vector3Dd *surfaceNormal, ColorRgba *color,
    const ColorRgba *lightColor, const ColorRgba *surfaceColor)
{
    double intensity;
    double roughness;

    Vector3Dd halfway = rEye.midpoint(lightSourceRay->getDirection());
    double normalLength = (*surfaceNormal).length();
    double halfwayLength = halfway.length();
    double cosAngleOfIncidence = halfway.dotProduct(*surfaceNormal);

    if (normalLength == 0.0 || halfwayLength == 0.0) {
        cosAngleOfIncidence = 0.0;
    } else {
        cosAngleOfIncidence /= (normalLength * halfwayLength);
    }

    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = 0.0;
    }

    roughness = 1.0 / texture->getObjectRoughness();

    if (roughness != 1.0) {
        intensity = java::Math::pow(cosAngleOfIncidence, roughness);
    } else {
        intensity = cosAngleOfIncidence;
    }
    intensity *= texture->getObjectSpecular();
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
