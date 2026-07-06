#include "render/shaders/LightSamplerShader.h"

void
LightSamplerShader::sample(const Light *lightSource, double *lightSourceDepth,
    RayWithTracingState *lightSourceRay, const Vector3Dd *intersectionPoint,
    ColorRgba *lightColor)
{
    double attenuation = 1.0;

    // Get the light source color
    *lightColor = lightSource->getShapeColor();

    lightSourceRay->setOrigin(*intersectionPoint);
    lightSourceRay->setQuadricConstantsCached(false);

    lightSourceRay->setDirection(
        lightSource->getPosition().subtract(*intersectionPoint));

    *lightSourceDepth = lightSourceRay->getDirection().length();

    lightSourceRay->setDirection(
        lightSourceRay->getDirection().multiply(1.0 / (*lightSourceDepth)));

    attenuation = lightSource->evaluateLightResponseFactor(lightSourceRay);

    // Now scale the color by the attenuation
    lightColor->setR(lightColor->getR() * attenuation);
    lightColor->setG(lightColor->getG() * attenuation);
    lightColor->setB(lightColor->getB() * attenuation);
}
