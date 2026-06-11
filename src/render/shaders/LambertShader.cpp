#include <cstdlib>
#include "java/lang/Math.h"
#include "environment/geometry/elements/RayWithSegments.h"
#include "render/shaders/LambertShader.h"

void
LambertShader::shade(Material *texture, RayWithSegments *lightSourceRay,
    Vector3Dd *surfaceNormal, ColorRgba *color, ColorRgba *lightColor,
    ColorRgba *surfaceColor, double attenuation)
{
    double cosAngleOfIncidence;
    double intensity;
    double randomNumber;

    cosAngleOfIncidence =
        (*surfaceNormal).dotProduct(lightSourceRay->direction);
    if (cosAngleOfIncidence < 0.0) {
        cosAngleOfIncidence = -cosAngleOfIncidence;
    }

    if (texture->objectBrilliance != 1.0) {
        intensity = java::Math::pow(cosAngleOfIncidence, texture->objectBrilliance);
    } else {
        intensity = cosAngleOfIncidence;
    }

    intensity *= texture->objectDiffuse * attenuation;

    randomNumber = (rand() & 0x7FFF) / (double)0x7FFF;

    intensity -= randomNumber * texture->textureRandomness;

    color->setR(color->getR() + intensity * (surfaceColor->getR()) * (lightColor->getR()));
    color->setG(color->getG() + intensity * (surfaceColor->getG()) * (lightColor->getG()));
    color->setB(color->getB() + intensity * (surfaceColor->getB()) * (lightColor->getB()));
}
