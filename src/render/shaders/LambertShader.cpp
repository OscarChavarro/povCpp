#include <cstdlib>

#include "java/lang/Math.h"
#include "environment/geometry/element/RayWithSegments.h"
#include "render/shaders/LambertShader.h"

void
LambertShader::shade(const PovrayMaterial *texture, const RayWithSegments *lightSourceRay,
    const Vector3Dd *surfaceNormal, ColorRgba *color, const ColorRgba *lightColor,
    const ColorRgba *surfaceColor, double attenuation)
{
    double cosAngleOfIncidence;
    double intensity;
    double randomNumber;

    cosAngleOfIncidence =
        (*surfaceNormal).dotProduct(lightSourceRay->getDirection());
    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = -cosAngleOfIncidence;
    }

    if (texture->getObjectBrilliance() != 1.0) {
        intensity = java::Math::pow(cosAngleOfIncidence, texture->getObjectBrilliance());
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->getObjectDiffuse() * attenuation;

    randomNumber = (rand() & 0x7FFF) / (double)0x7FFF;

    intensity -= randomNumber * texture->getTextureRandomness();

    color->setR(color->getR() + intensity * (surfaceColor->getR()) * (lightColor->getR()));
    color->setG(color->getG() + intensity * (surfaceColor->getG()) * (lightColor->getG()));
    color->setB(color->getB() + intensity * (surfaceColor->getB()) * (lightColor->getB()));
}
